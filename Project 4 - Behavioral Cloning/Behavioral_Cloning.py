import csv
import cv2
import numpy as np

print("RUNNING")
STEERING_CORRECTION = 0.23

lines = []
with open('./data/driving_log.csv') as csvfile:
	reader = csv.reader(csvfile)
	for line in reader:
		lines.append(line)

images = []
measurements = []
run_once = 0

for line in lines:
	for i in range(3):
		source_path = line[i]
		filename = source_path.split('\\')[-1]
		current_path = './data/IMG/' + filename
		image = cv2.imread(current_path)
		images.append(image)
	measurement = float(line[3])
	measurements.append(measurement)
	measurements.append(measurement + STEERING_CORRECTION)
	measurements.append(measurement - STEERING_CORRECTION)
	

augmented_images, augmented_measurements = [], []
for image, measurement in zip(images, measurements):
	augmented_images.append(image)
	augmented_measurements.append(measurement)
	augmented_images.append(cv2.flip(image, 1))
	augmented_measurements.append(measurement * -1.0)

X_train = np.array(augmented_images)
image_shape = X_train[0].shape
y_train = np.array(augmented_measurements)

from keras.models import Sequential
from keras.layers import Flatten, Dense, Lambda, Cropping2D
from keras.layers import Convolution2D
from keras.layers import MaxPooling2D
from keras.layers import Dropout

model = Sequential()
model.add(Lambda(lambda x: x / 255.0 - 0.5, input_shape=image_shape))
model.add(Cropping2D(cropping=((70, 25), (0, 0))))

model.add(Convolution2D(24, 5, 5, border_mode='same', subsample=(1, 1), activation='relu'))
model.add(MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))
model.add(Convolution2D(36, 5, 5, border_mode='same', subsample=(1, 1), activation='relu'))
model.add(MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))
model.add(Convolution2D(48, 5, 5, border_mode='same', subsample=(1, 1), activation='relu'))
model.add(MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))
model.add(Convolution2D(64, 3, 3, border_mode='same', subsample=(1, 1), activation='relu'))
model.add(Convolution2D(64, 3, 3, border_mode='same', subsample=(1, 1), activation='relu'))

model.add(Flatten())
model.add(Dense(1164, activation='relu'))
model.add(Dropout(0.3))
model.add(Dense(100, activation='relu'))
model.add(Dropout(0.3))
model.add(Dense(50, activation='relu'))
model.add(Dense(10, activation='relu'))
model.add(Dense(1))

model.summary()

model.compile(loss='mse', optimizer='adam')
model.fit(X_train, y_train, validation_split=0.2, shuffle=True, epochs=2)

model.save('model.h5')
