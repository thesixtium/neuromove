import cv2
import os
import joblib
import threading
import numpy as np
from collections import deque

import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

from src.RaspberryPi.SharedMemory import SharedMemory
from src.RaspberryPi.InternalException import EyeTrackingNoRet


class EyeTracking:
    def __init__(self):
        self.eye_tracking_memory = SharedMemory(shem_name="eye_tracking", size=10, create=False)
        self.model = joblib.load(os.path.join("src", "RaspberryPi", "GeneralizedModel3Compress9"))

        # Create a FaceLandmarker object.
        base_options = python.BaseOptions(model_asset_path=os.path.join("src", "RaspberryPi", "face_landmarker2.task"))
        options = vision.FaceLandmarkerOptions(base_options=base_options,
                                               output_face_blendshapes=True,
                                               output_facial_transformation_matrixes=True,
                                               num_faces=1)
        self.detector = vision.FaceLandmarker.create_from_options(options)

        # Real-time camera capture with sliding window prediction.
        self.cap = cv2.VideoCapture(0)  # 0 for default camera

        # Initialize a deque (double-ended queue) to store the features from the last 30 frames.
        self.window_size = 5 #set to predict 1 time every 30s
        self.prediction_threshold = 0.43

        self.feature_window = deque(maxlen=self.window_size)

        self.eye_tracking_thread_running = True
        self.eye_tracking_thread = threading.Thread(target=self.serial_read)
        self.eye_tracking_thread.start()

    def close(self):
        self.eye_tracking_thread_running = False
        self.cap.release()
        cv2.destroyAllWindows()

    def serial_read(self):
        while self.eye_tracking_thread_running:
            ret, frame = self.cap.read()
            if not ret:
                raise EyeTrackingNoRet()

            # Convert the frame to RGB format
            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            frame_rgb_uint8 = np.array(frame_rgb, dtype=np.uint8)
            image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame_rgb_uint8)

            # Perform face detection with the converted image
            detection_result = self.detector.detect(image)

            if detection_result.face_landmarks:
                #Extract Landmarks and calculate angle features
                #Collect coordinates for middle of forehead (origin)
                Forehead_Origin = np.array([detection_result.face_landmarks[0][151].x, detection_result.face_landmarks[0][151].y, detection_result.face_landmarks[0][151].z])

                #Collect coordinates for the point directly to the right of the origin point
                Forehead_Horizontal = np.array([detection_result.face_landmarks[0][337].x, detection_result.face_landmarks[0][337].y, detection_result.face_landmarks[0][337].z])

                #Collect coordinates for the point directly above the origin point
                Forehead_Vertical = np.array([detection_result.face_landmarks[0][10].x, detection_result.face_landmarks[0][10].y, detection_result.face_landmarks[0][10].z])

                #Calculate the 3-axis Tilt
                Forehead_Cross = np.cross(Forehead_Horizontal-Forehead_Origin, Forehead_Vertical-Forehead_Origin)
                Norm_Forehead_Cross = Forehead_Cross / np.linalg.norm(Forehead_Cross)
                Angles = np.abs(np.arcsin(Norm_Forehead_Cross))

                # Extract blenshape and angle features for the model
                features = [detection_result.face_blendshapes[0][i].score for i in range(9, 23)]
                features.append(Angles[1])
                features.append(Angles[0])
                features.append(Angles[2])
                self.feature_window.append(features)

                # If we have enough frames in the window, make a prediction
                if len(self.feature_window) == self.window_size:
                    # Average the features over the window
                    averaged_features = np.mean(self.feature_window, axis=0).reshape(1, -1)

                    #predict threshold then predic using modified threshold
                    y_pred_test = self.model.predict_proba(averaged_features)
                    prediction = (y_pred_test[:, 1] >= self.prediction_threshold).astype(int)
                    #prediction = 1
                    self.eye_tracking_memory.write_string(str(prediction))
                    print(f"Eye Tracking: {prediction}")
