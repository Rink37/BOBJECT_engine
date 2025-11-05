import numpy as np
import cv2
import os

fileName = "D2NButton.png"
nameNoExt = fileName.split(".")[0]
ext = fileName.split(".")[1]

if os.path.exists(fileName):
    image = cv2.imread(fileName)
    image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY).astype("float64")
    image -= np.min(image)
    image *= 255/np.max(image)
    image = image.astype("uint8")
    cv2.imwrite(nameNoExt+"_gray."+ext, image)