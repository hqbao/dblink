import cv2 as cv
import time

stream1 = cv.VideoCapture('http://192.168.4.1:81/stream')

t = time.time()

print('Start video')
while True:
  ret1, frame1 = stream1.read()
  if ret1:
    cv.imshow('Cam1', frame1)

  if cv.waitKey(10) & 0xFF == ord('q'):
    break

stream1.release()
cv.destroyAllWindows()
print('Video stopped')
