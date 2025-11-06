import numpy as np
import cv2
import os

fileNames = os.listdir()
print(fileNames)


#fileName = "D2NButton.png"
for fileName in fileNames:
    if len(fileName.split(".")) == 1:
        continue
    nameNoExt = fileName.split(".")[0]
    ext = fileName.split(".")[1]
    
    if ext == "png" and os.path.exists(fileName):
        image = cv2.imread(fileName)
        image = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY).astype("float64")
        image -= np.min(image)
        image *= 255/np.max(image)
        image = image.astype("uint8")
        cv2.imwrite(nameNoExt+"."+ext, image)