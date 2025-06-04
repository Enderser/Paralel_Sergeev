import cv2
import threading
import queue
import argparse
import logging
import logging.config
import time
import numpy as np
import os

# Загрузка конфигурации логирования
logging.config.fileConfig('logging.conf', disable_existing_loggers=False)

class Sensor:
    def __init__(self):
        pass

    def get(self):
        raise NotImplementedError("Subclasses must implement method get()")

    def __del__(self):
        pass

class SensorX(Sensor):
    def __init__(self, delay):
        """Инициализация датчика с заданной задержкой в секундах."""
        self._delay = delay
        self._data = 0
        self._last_time = time.time()

    def get(self):
        """Получение значения датчика с учетом задержки."""
        current_time = time.time()
        if current_time - self._last_time >= self._delay:
            self._data += 1
            self._last_time = current_time
        return self._data

class SensorCam(Sensor):
    def __init__(self, camera_name, resolution):
        super().__init__()
        try:
            self.cap = cv2.VideoCapture(camera_name)
            if not self.cap.isOpened():
                raise ValueError(f"Cannot open camera: {camera_name}")
            width, height = map(int, resolution.split('x'))
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
            self.frame_queue = queue.Queue(maxsize=1)
            self.running = True
            self.thread = threading.Thread(target=self._read_frames, daemon=True)
        except Exception as e:
            logging.error(f"SensorCam initialization failed: {str(e)}")
            raise
        self.thread.start()

    def _read_frames(self):
        while self.running:
            ret, frame = self.cap.read()
            if not ret:
                logging.error("Failed to read frame from camera")
                continue
            try:
                self.frame_queue.put_nowait(frame)
            except queue.Full:
                pass
            time.sleep(0.001)

    def get(self):
        try:
            return self.frame_queue.get_nowait()
        except queue.Empty:
            return None

    def __del__(self):
        self.running = False
        if hasattr(self, 'thread'):
            self.thread.join()
        if hasattr(self, 'cap') and self.cap.isOpened():
            self.cap.release()

class SensorXWrapper(Sensor):
    def __init__(self, delay):
        super().__init__()
        try:
            self.sensor = SensorX(delay)
            self.data_queue = queue.Queue(maxsize=1)
            self.running = True
            self.thread = threading.Thread(target=self._read_sensor, daemon=True)
        except Exception as e:
            logging.error(f"SensorXWrapper initialization failed: {str(e)}")
            raise
        self.thread.start()

    def _read_sensor(self):
        while self.running:
            data = self.sensor.get()
            try:
                self.data_queue.put_nowait(data)
            except queue.Full:
                pass
            time.sleep(0.001)

    def get(self):
        try:
            return self.data_queue.get_nowait()
        except queue.Empty:
            return None

    def __del__(self):
        self.running = False
        if hasattr(self, 'thread'):
            self.thread.join()

class WindowImage:
    def __init__(self, display_frequency):
        self.window_name = "Sensor Data"
        self.display_interval = 1.0 / display_frequency
        try:
            cv2.namedWindow(self.window_name)
        except Exception as e:
            logging.error(f"WindowImage initialization failed: {str(e)}")
            raise
        self.last_display_time = time.time()

    def show(self, img, sensor_data):
        try:
            display_img = img.copy() if img is not None else np.zeros((480, 640, 3), dtype=np.uint8)
            for i, data in enumerate(sensor_data):
                text = f"Sensor {i}: {data}" if data is not None else f"Sensor {i}: No data"
                cv2.putText(display_img, text, (10, 30 + i * 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
            cv2.imshow(self.window_name, display_img)
            cv2.waitKey(1)
        except Exception as e:
            logging.error(f"WindowImage show failed: {str(e)}")

    def __del__(self):
        try:
            cv2.destroyWindow(self.window_name)
        except Exception as e:
            logging.error(f"WindowImage cleanup failed: {str(e)}")

def parse_args():
    parser = argparse.ArgumentParser(description="Sensor data processing")
    parser.add_argument('--camera_name', type=str, default='/dev/video0', help='Camera device name')
    parser.add_argument('--resolution', type=str, default='1280x720', help='Camera resolution (e.g., 1280x720)')
    parser.add_argument('--display_freq', type=float, default=30.0, help='Display frequency in Hz')
    return parser.parse_args()

def main():
    args = parse_args()
    
    try:
        cam = SensorCam(args.camera_name, args.resolution)
        sensors = [
            SensorXWrapper(0.01),  # 100 Hz
            SensorXWrapper(0.1),   # 10 Hz
            SensorXWrapper(1.0)    # 1 Hz
        ]
        window = WindowImage(args.display_freq)
        
        last_frame = None
        last_sensor_data = [None, None, None]
        
        while True:
            frame = cam.get()
            if frame is not None:
                last_frame = frame
            
            for i, sensor in enumerate(sensors):
                data = sensor.get()
                if data is not None:
                    last_sensor_data[i] = data
            
            current_time = time.time()
            if current_time - window.last_display_time >= window.display_interval:
                if last_frame is not None:
                    window.show(last_frame, last_sensor_data)
                window.last_display_time = current_time
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except Exception as e:
        logging.error(f"Main loop error: {str(e)}")
        raise
    finally:
        pass  # Ресурсы освобождаются автоматически через деструкторы

if __name__ == "__main__":
    main()