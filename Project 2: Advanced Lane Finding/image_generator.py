import numpy as np
import cv2
import glob
import pickle
from Tracker import Tracker

dist_pickle = pickle.load(open('calibration_pickle.p', 'rb'))
mtx = dist_pickle['mtx']
dist = dist_pickle['dist']


def abs_sobel_thresh(img, orient='x', sobel_kernel=3, thresh=(0, 255)):
	# 1) Convert to grayscale
	gray = cv2.cvtColor(img, cv2.COLOR_RGB2GRAY)
	# 2) Take the derivative in x or y given orient = 'x' or 'y'
	if orient == 'x':
		sobel = cv2.Sobel(gray, cv2.CV_64F, 1, 0, ksize=sobel_kernel)
	if orient == 'y':
		sobel = cv2.Sobel(gray, cv2.CV_64F, 0, 1, ksize=sobel_kernel)
	# 3) Take the absolute value of the derivative or gradient
	abs_sobel = np.absolute(sobel)
	# 4) Scale to 8-bit (0 - 255) then convert to type = np.uint8
	scaled_sobel = np.uint8(255 * abs_sobel / np.max(abs_sobel))
	# 5) Create a mask of 1's where the scaled gradient magnitude is > thresh_min and < thresh_max
	binary_output = np.zeros_like(scaled_sobel)
	binary_output[(scaled_sobel >= thresh[0]) & (scaled_sobel <= thresh[1])] = 1
	# 6) Return this mask as your binary_output image
	return binary_output


def mag_thresh(image, sobel_kernel=3, mag_thresh=(0, 255)):
	gray = cv2.cvtColor(image, cv2.COLOR_RGB2GRAY)
	sobelx = cv2.Sobel(gray, cv2.CV_64F, 1, 0, ksize=sobel_kernel)
	sobely = cv2.Sobel(gray, cv2.CV_64F, 0, 1, ksize=sobel_kernel)
	gradmag = np.sqrt(sobelx ** 2 + sobely ** 2)
	scale_factor = np.max(gradmag) / 255
	gradmag = (gradmag / scale_factor).astype(np.uint8)
	binary_output = np.zeros_like(gradmag)
	binary_output[(gradmag >= mag_thresh[0]) & (gradmag <= mag_thresh[1])] = 1
	return binary_output


def dir_threshold(image, sobel_kernel=3, thresh=(0, np.pi / 2)):
	gray = cv2.cvtColor(image, cv2.COLOR_RGB2GRAY)
	sobelx = cv2.Sobel(gray, cv2.CV_64F, 1, 0, ksize=sobel_kernel)
	sobely = cv2.Sobel(gray, cv2.CV_64F, 0, 1, ksize=sobel_kernel)
	with np.errstate(divide='ignore', invalid='ignore'):
		abs_gradient_direction = np.absolute(np.arctan(sobely / sobelx))
		binary_output = np.zeros_like(abs_gradient_direction)
		binary_output[(abs_gradient_direction >= thresh[0]) & (abs_gradient_direction <= thresh[1])] = 1
	return binary_output


def color_threshold(image, sthresh=(0, 255), vthresh=(0, 255)):
	hls = cv2.cvtColor(image, cv2.COLOR_RGB2HLS)
	s_channel = hls[:, :, 2]
	s_binary = np.zeros_like(s_channel)
	s_binary[(s_channel >= sthresh[0]) & (s_channel <= sthresh[1])] = 1
	
	hsv = cv2.cvtColor(image, cv2.COLOR_RGB2HSV)
	v_channel = hsv[:, :, 2]
	v_binary = np.zeros_like(v_channel)
	v_binary[(v_channel >= vthresh[0]) & (v_channel <= vthresh[1])] = 1
	
	output = np.zeros_like(s_channel)
	output[(s_binary == 1) & (v_binary == 1)] = 1
	return output


def window_mask(width, height, img_ref, center, level):
	output = np.zeros_like(img_ref)
	output[int(img_ref.shape[0] - (level + 1) * height):int(img_ref.shape[0] - level * height), max(0, int(center - width)):min(int(center + width), img_ref.shape[1])] = 1
	return output


images = glob.glob('test_images/test*.jpg')

# iterate through the list and search for chessboard corners
for idx, file_name in enumerate(images):
	print('Working on ' + file_name)
	# read the image
	img = cv2.imread(file_name)
	
	# undistort the imaage
	undistorted_img = cv2.undistort(img, mtx, dist, None, mtx)
	
	# process image and generate binary pixel of interest
	binary_output = np.zeros_like(undistorted_img[:, :, 0])
	gradx = abs_sobel_thresh(undistorted_img, orient='x', thresh=(12, 255))
	grady = abs_sobel_thresh(undistorted_img, orient='y', thresh=(25, 255))
	c_binary = color_threshold(undistorted_img, sthresh=(100, 255), vthresh=(50, 255))
	binary_output[((gradx == 1) & (grady == 1) | (c_binary == 1))] = 255
	
	# defining perspective transformation area
	img_size = (binary_output.shape[1], binary_output.shape[0])
	bot_width = 0.76  # percent of bottom trapezoid height
	mid_width = 0.09  # percent of middle trapezoid height
	height_pct = 0.62  # percent of trapezoid height
	bottom_trim = 0.935  # percent from top to bottom to avoid car hood
	src = np.float32(
		[
			[img_size[0] * (0.5 - mid_width / 2), img_size[1] * height_pct],
			[img_size[0] * (0.5 + mid_width / 2), img_size[1] * height_pct],
			[img_size[0] * (0.5 - bot_width / 2), img_size[1] * bottom_trim],
			[img_size[0] * (0.5 + bot_width / 2), img_size[1] * bottom_trim],
		]
	)
	
	offset = img_size[0] * 0.25
	
	dst = np.float32(
		[
			[offset, 0],
			[img_size[0] - offset, 0],
			[offset, img_size[1]],
			[img_size[0] - offset, img_size[1]],
		]
	)
	
	M = cv2.getPerspectiveTransform(src, dst)
	Minv = cv2.getPerspectiveTransform(dst, src)
	warped = cv2.warpPerspective(binary_output, M, img_size, flags=cv2.INTER_LINEAR)
	
	window_width = 25
	window_height = 80
	# 720 pixels correspond to 10 meters in the y direction
	ym = 10 / 720
	# 384 pixels correspond to 4 meters in the x direction
	xm = 4 / 384
	
	# set up the overall class to do all the tracking
	curve_centers = Tracker(Mywindow_width=window_width, Mywindow_height=window_height, Mymargin=25, My_ym=ym, My_xm=xm, Mysmooth_factor=15)
	window_centroids = curve_centers.find_window_centroids(warped)
	
	# points used to draw all the left and the right windows
	l_points = np.zeros_like(warped)
	r_points = np.zeros_like(warped)
	
	# Points used to find the left and right lanes
	rightx = []
	leftx = []
	
	# go through each level and draw the windows
	for level in range(0, len(window_centroids)):
		# window_mask is a function to draw window areas
		leftx.append(window_centroids[level][0])
		rightx.append(window_centroids[level][1])
		
		l_mask = window_mask(window_width, window_height, warped, window_centroids[level][0], level)
		r_mask = window_mask(window_width, window_height, warped, window_centroids[level][1], level)
		# add graphic points from window mask here to total pixel found
		l_points[(l_points == 255) | (l_mask == 1)] = 255
		r_points[(r_points == 255) | (r_mask == 1)] = 255
	
	# draw the results
	template = np.array(r_points + l_points, np.uint8)  # add both left and right window pixels together
	zero_channel = np.zeros_like(template)  # create a zero color channel
	template = np.array(cv2.merge((zero_channel, template, zero_channel)), np.uint8)  # make a window pixels green
	warpage = np.array(cv2.merge((warped, warped, warped)), np.uint8)  # making the original road pixels 3 color channels
	result = cv2.addWeighted(warpage, 1, template, 0.5, 0.0)  # overlay the original road image with window results
	
	# fit the lane boundaries to the left, right center positions found
	yvals = range(0, warped.shape[0])
	
	res_yvals = np.arange(warped.shape[0] - (window_height / 2), 0, -window_height)
	
	left_fit = np.polyfit(res_yvals, leftx, 2)
	left_fitx = left_fit[0] * yvals * yvals + left_fit[1] * yvals + left_fit[2]
	left_fitx = np.array(left_fitx, np.int32)
	
	right_fit = np.polyfit(res_yvals, rightx, 2)
	right_fitx = right_fit[0] * yvals * yvals + right_fit[1] * yvals + right_fit[2]
	right_fitx = np.array(right_fitx, np.int32)
	
	left_lane = np.array(list(zip(np.concatenate((left_fitx - window_width / 2, left_fitx[::-1] + window_width / 2), axis=0), np.concatenate((yvals, yvals[::-1]), axis=0))), np.int32)
	right_lane = np.array(list(zip(np.concatenate((right_fitx - window_width / 2, right_fitx[::-1] + window_width / 2), axis=0), np.concatenate((yvals, yvals[::-1]), axis=0))), np.int32)
	inner_lane = np.array(list(zip(np.concatenate((left_fitx + window_width / 2, right_fitx[::-1] - window_width / 2), axis=0), np.concatenate((yvals, yvals[::-1]), axis=0))), np.int32)
	
	road = np.zeros_like(undistorted_img)
	road_bkg = np.zeros_like(undistorted_img)
	
	cv2.fillPoly(road, [left_lane], color=[255, 0, 0])
	cv2.fillPoly(road, [right_lane], color=[0, 0, 255])
	cv2.fillPoly(road, [inner_lane], color=[0, 255, 0])
	cv2.fillPoly(road_bkg, [left_lane], color=[255, 255, 255])
	cv2.fillPoly(road_bkg, [right_lane], color=[255, 255, 255])
	
	road_warped = cv2.warpPerspective(road, Minv, img_size, flags=cv2.INTER_LINEAR)
	road_warped_bkg = cv2.warpPerspective(road_bkg, Minv, img_size, flags=cv2.INTER_LINEAR)
	
	base = cv2.addWeighted(undistorted_img, 1.0, road_warped_bkg, -0.4, 0.0)
	result = cv2.addWeighted(base, 1.0, road_warped, 0.7, 0.0)
	
	# TODO: should I just use ym and xm?
	ym_per_pix = curve_centers.ym_per_pix  # meters per pixel in y dimension
	xm_per_pix = curve_centers.xm_per_pix  # meters per pixel in x dimension
	
	# using the curvature of the left line
	# TODO: should I use both right and left line?
	curve_fit_cr = np.polyfit(np.array(res_yvals, np.float32) * ym_per_pix, np.array(leftx, np.float32) * xm_per_pix, 2)
	curverad = ((1 + (2 * curve_fit_cr[0] * yvals[-1] * ym_per_pix + curve_fit_cr[1] ** 2) ** 1.5) / np.absolute(2 * curve_fit_cr[0]))
	
	# calculate the offset of the car on the road
	camera_center = (left_fitx[-1] + right_fitx[-1]) / 2
	center_diff = (camera_center - warped.shape[1] / 2) * xm_per_pix
	side_pos = 'left'
	# TODO: Investigate if 'right' is ever displayed
	if center_diff <= 0:
		side_pos = 'right'
	
	# draw the text showing curvature, offset, and speed
	cv2.putText(result, 'Radius of curvature = ' + str(round(curverad, 3)) + 'm', (50, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
	cv2.putText(result, 'Vehicle is ' + str(abs(round(center_diff, 3))) + 'm ' + side_pos + ' of center', (50, 100), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
	
	# result = road
	
	write_location = 'test_images_output/tracked_' + str(file_name)[len('test_images/'):]
	cv2.imwrite(write_location, result)
