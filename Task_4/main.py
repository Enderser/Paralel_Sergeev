import os
import threading
import queue
import cv2
import logging
import logging.config
import argparse
import time
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass


os.makedirs('log', exist_ok=True)

logging.config.fileConfig('logging.conf')
logger = logging.getLogger(__name__)

class Sensor:
    def get(self):
        raise NotImplementedError

class SensorCam(Sensor):
    def __init__(self, camera_name, resolution):
        self.camera_name = camera_name
        self.resolution = resolution
        self.cap = cv2.VideoCapture(camera_name)
        if not self.cap.isOpened():
            logger.error(f"Ошибка: камера {camera_name} не найдена")
            raise Exception("Камера не доступна")
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, int(resolution.split('x')[0]))
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, int(resolution.split('x')[1]))

    def get(self):
        ret, frame = self.cap.read()
        if not ret:
            logger.error("Ошибка чтения кадра с камеры")
            return None
        return frame

    def __del__(self):
        if self.cap.isOpened():
            self.cap.release()

class WindowImage:
    def __init__(self, display_freq):
        self.display_freq = display_freq
        self.delay = 1.0 / display_freq

    def show(self, img):
        cv2.imshow('Sensor Data', img)
        cv2.waitKey(int(self.delay * 1000))

    def __del__(self):
        cv2.destroyWindow('Sensor Data')

def sensor_thread(sensor, data_queue, freq):
    while True:
        start_time = time.time()
        try:
            data = sensor.get()
            data_queue.put((data, time.time() - start_time))
        except Exception as e:
            logger.error(f"Ошибка в потоке датчика: {e}")
        sleep_time = max(0, 1.0 / freq - (time.time() - start_time))
        time.sleep(sleep_time)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--camera', default='/dev/video0')
    parser.add_argument('--resolution', default='1280x720')
    parser.add_argument('--freq', type=float, default=30.0)
    args = parser.parse_args()

    try:
        cam = SensorCam(args.camera, args.resolution)
        sensor0 = SensorX(0.01)  # 100 Гц
        sensor1 = SensorX(0.1)   # 10 Гц
        sensor2 = SensorX(1)     # 1 Гц
    except Exception as e:
        logger.error(f"Ошибка инициализации датчиков: {e}")
        return

    queues = {
        'cam': queue.Queue(),
        'sensor0': queue.Queue(),
        'sensor1': queue.Queue(),
        'sensor2': queue.Queue()
    }

    with ThreadPoolExecutor(max_workers=4) as executor:
        executor.submit(sensor_thread, cam, queues['cam'], 30)
        executor.submit(sensor_thread, sensor0, queues['sensor0'], 100)
        executor.submit(sensor_thread, sensor1, queues['sensor1'], 10)
        executor.submit(sensor_thread, sensor2, queues['sensor2'], 1)

        window = WindowImage(args.freq)
        last_data = {'cam': None, 'sensor0': None, 'sensor1': None, 'sensor2': None}
        while True:
            try:
                cam_data, cam_delay = queues['cam'].get_nowait()
                s0_data, s0_delay = queues['sensor0'].get_nowait()
                s1_data, s1_delay = queues['sensor1'].get_nowait()
                s2_data, s2_delay = queues['sensor2'].get_nowait()

                last_data['cam'] = cam_data if cam_data else last_data['cam']
                last_data['sensor0'] = s0_data if s0_data else last_data['sensor0']
                last_data['sensor1'] = s1_data if s1_data else last_data['sensor1']
                last_data['sensor2'] = s2_data if s2_data else last_data['sensor2']

                img = last_data['cam'].copy() if last_data['cam'] is not None else None
                if img is not None:
                    window.show(img)
                logger.info(f"Задержки: cam={cam_delay:.4f}, s0={s0_delay:.4f}, s1={s1_delay:.4f}, s2={s2_delay:.4f}")

            except queue.Empty:
                if last_data['cam'] is not None:
                    window.show(last_data['cam'])

            except Exception as e:
                logger.error(f"Ошибка в основном цикле: {e}")

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    del cam
    del window

if __name__ == "__main__":
    main()