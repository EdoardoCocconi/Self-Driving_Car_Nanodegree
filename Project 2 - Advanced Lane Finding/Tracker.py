import numpy as np


class Tracker:
	def __init__(self, Mywindow_width, Mywindow_height, Mymargin, Mysmooth_factor):
		# list that stores all the past (left, right) center set values used for smoothing the output
		self.recent_centers = []
		# the window pixel width of the center values, used to count pixels inside center windows to determine curve values
		self.window_width = Mywindow_width
		# the window pixel height of the center values, used to count pixels inside center winndows to determine curve values
		# breaks the image into verticallevels
		self.window_height = Mywindow_height
		# the pixel distance in both directions to slide (left_window + right window) template for searching
		self.margin = Mymargin
		# How many frames results are averaged together to find the current frame result
		self.smooth_factor = Mysmooth_factor
	
	# the main tracking function for finding and storing lane segment positions
	def find_window_centroids(self, warped):
		# store the (left, right) window centroid positions per level
		window_centroids = []
		# create our window template that we will use for convolutions
		window = np.ones(self.window_width)
		
		# first find the two starting positions for the left and right lane by using np.sum to get the vertical image slice
		# and then np.convolute the vertical image slice with the window template
		
		# sum quarter bottom of image to get slice, could use a different ratio
		if not self.recent_centers:
			l_sum = np.sum(warped[int(3 * warped.shape[0] / 4):, :int(warped.shape[1] / 2)], axis=0)
			r_sum = np.sum(warped[int(3 * warped.shape[0] / 4):, int(warped.shape[1] / 2):], axis=0)
			l_center = np.argmax(np.convolve(window, l_sum)) - self.window_width / 2
			r_center = np.argmax(np.convolve(window, r_sum)) - self.window_width / 2 + int(warped.shape[1] / 2)
		else:
			l_sum = np.sum(warped[int(3 * warped.shape[0] / 4):, (max(int(self.recent_centers[-1][0][0]) - int(self.margin), 0)):(int(self.recent_centers[-1][0][0]) + int(self.margin))], axis=0)
			r_sum = np.sum(warped[int(3 * warped.shape[0] / 4):, (max(int(self.recent_centers[-1][0][1]) - int(self.margin), 0)):(int(self.recent_centers[-1][0][1]) + int(self.margin))], axis=0)
			l_center = np.argmax(np.convolve(window, l_sum)) - self.window_width / 2 + int(self.recent_centers[-1][0][0]) - int(self.margin)
			r_center = np.argmax(np.convolve(window, r_sum)) - self.window_width / 2 + int(self.recent_centers[-1][0][1]) - int(self.margin)
		
		# add what we found for the first layer
		window_centroids.append((l_center, r_center))
		
		# go through each layer looking for max pixel locations
		for level in range(1, int(warped.shape[0] / self.window_height)):
			# find the best left centroid by using past left center as a reference
			# use window_width/2 as offset because convolution signal reference is at right side of window, not center of window
			offset = self.window_width / 2
			l_min_index = int(max(l_center + offset - self.margin, 0))
			l_max_index = int(min(l_center + offset + self.margin, warped.shape[1]))
			# find the best right centroid by using past right cebter as a reference
			r_min_index = int(max(r_center + offset - self.margin, 0))
			r_max_index = int(min(r_center + offset + self.margin, warped.shape[1]))
			
			# convolve the window into the vertical slice of the image
			image_layer_left = np.sum(warped[int(warped.shape[0] - (level + 1) * self.window_height):int(warped.shape[0] - level * self.window_height), l_min_index:l_max_index], axis=0)
			image_layer_right = np.sum(warped[int(warped.shape[0] - (level + 1) * self.window_height):int(warped.shape[0] - level * self.window_height), r_min_index:r_max_index], axis=0)
			conv_signal_left = np.convolve(window, image_layer_left)
			conv_signal_right = np.convolve(window, image_layer_right)
			
			if np.count_nonzero(conv_signal_left[:]) != 0:
				l_center = np.argmax(conv_signal_left[:]) + l_min_index - offset

			if np.count_nonzero(conv_signal_right[:]) != 0:
				r_center = np.argmax(conv_signal_right[:]) + r_min_index - offset
			# add what we found for that layer
			window_centroids.append((l_center, r_center))
		
		self.recent_centers.append(window_centroids)
		# return averaged values of the line centers. helps to keep the markers from jumping around too much
		return np.average(self.recent_centers[-self.smooth_factor:], axis=0)
