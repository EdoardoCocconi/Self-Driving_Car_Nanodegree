import numpy as np
import cv2
import glob
import pickle


nx = 9
ny = 6

objpoints = []  # 3D points in real world space
imgpoints = []  # 2D points in image plane
objp = np.zeros((ny * nx, 3), np.float32)
objp[:, :2] = np.mgrid[0:nx, 0:ny].T.reshape(-1, 2)

# make a list of calibration images
images = glob.glob('camera_cal/calibration*.jpg')

# iterate through the list and search for chessboard corners
for idx, file_name in enumerate(images):
	img = cv2.imread(file_name)
	# Convert to grayscale
	gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
	# Find the chessboard corners
	ret, corners = cv2.findChessboardCorners(gray, (nx, ny), None)
	# if chessboard corners have been found, add obj points and img points
	if ret:
		print('Working on ' + file_name)
		imgpoints.append(corners)
		objpoints.append(objp)
		# draw the corners
		cv2.drawChessboardCorners(img, (nx, ny), corners, ret)
		# save the images at the desired location. The 'camera_cal/' part of file_name is removed
		write_location = 'camera_cal_corners/corners_found_' + str(file_name)[len('camera_cal/'):]
		

# get the image size
file_name = 'camera_cal/calibration1.jpg'
img = cv2.imread(file_name)
img_size = (img.shape[1], img.shape[0])

ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(objpoints, imgpoints, img_size, None, None)
undistorted_img = cv2.undistort(img, mtx, dist, None, mtx)
write_location = 'camera_cal_undistorted/' + str(file_name)[len('camera_cal/'):]
cv2.imwrite(write_location, undistorted_img)

# save the camera calibration result for later use
dist_pickle = {'mtx': mtx, 'dist': dist}
pickle.dump(dist_pickle, open('calibration_pickle.p', 'wb'))
