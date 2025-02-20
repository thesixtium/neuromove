#Import the necessary modules.
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe import solutions
from mediapipe.framework.formats import landmark_pb2
import numpy as np
import cv2
from collections import deque
from sklearn.ensemble import RandomForestClassifier
import pandas as pd
import pickle


def main(side, outputvid):
    with open('GeneralizedModel.pkl','rb') as file:
        model = pickle.load(file)

    # Create a FaceLandmarker object.
    base_options = python.BaseOptions(model_asset_path='face_landmarker2.task')
    options = vision.FaceLandmarkerOptions(base_options=base_options,
                                        output_face_blendshapes=True,
                                        output_facial_transformation_matrixes=True,
                                        num_faces=1)
    detector = vision.FaceLandmarker.create_from_options(options)

    # Real-time camera capture with sliding window prediction.
    if side == 2:   
        cap = cv2.VideoCapture(0)  # 0 for default camera
        cap = cv2.flip(cap,1)

    else:
        cap = cv2.VideoCapture(0)  # 0 for default camera

    # Initialize a deque (double-ended queue) to store the features from the last 5 frames.
    window_size = 5 #set to predict 1 time every 0.167s
    prediction_size = 6 #set to use the average of 6 predictions (outputs averaged prediction 1 time every s)
    feature_window = deque(maxlen=window_size)
    prediction_window = deque(maxlen=prediction_size)

    while cap.isOpened():
        ret, frame = cap.read()
        if not ret:
            break

        # Convert the frame to RGB format
        frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        frame_rgb_uint8 = np.array(frame_rgb, dtype=np.uint8)
        image = mp.Image(image_format=mp.ImageFormat.SRGB, data=frame_rgb_uint8)

        # Perform face detection with the converted image
        detection_result = detector.detect(image)

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
            feature_window.append(features)

            # If we have enough frames in the window, make a prediction
            if len(feature_window) == window_size:
                # Average the features over the window
                averaged_features = np.mean(feature_window, axis=0).reshape(1, -1)

                # Make a prediction
                # prediction = model.predict(averaged_features)

                #predict threshold then predic using modified threshold
                y_pred_test = model.predict_proba(averaged_features)
                custom_threshold = 0.43  # Set your desired threshold
                prediction = (y_pred_test[:, 1] >= custom_threshold).astype(int)
                prediction_window.append(prediction)
                
                if outputvid:
                    # Annotate the frame with landmarks
                    annotated_image = draw_landmarks_on_image(frame_rgb, detection_result)
                    annotated_image_bgr = cv2.cvtColor(annotated_image, cv2.COLOR_RGB2BGR)

                    # Display the annotated frame with prediction text
                    cv2.putText(annotated_image_bgr, f"Prediction: {prediction[0]}", (10, 30),cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2, cv2.LINE_AA)
                    cv2.imshow('Annotated Image', annotated_image_bgr)

                if len(prediction_window) == prediction_size:
                    prediction_sum = sum(prediction_window)
                    averaged_prediction = prediction_sum/prediction_size
                    
                    if averaged_prediction >= 0.5:
                        return 1
                    
                    else:
                        return 0

            else:
                if outputvid:
                    cv2.imshow('Camera Feed', frame)
    
        else:
            if outputvid:
                cv2.imshow('Camera Feed', frame)
                
            # no landmarks in video so output 0
            return 0

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

#Function to draw the face landmarks on the image for output (not necessary but still included it).
def draw_landmarks_on_image(rgb_image, detection_result):
    face_landmarks_list = detection_result.face_landmarks
    annotated_image = np.copy(rgb_image)

    for idx in range(len(face_landmarks_list)):
        face_landmarks = face_landmarks_list[idx]
        face_landmarks_proto = landmark_pb2.NormalizedLandmarkList()
        face_landmarks_proto.landmark.extend([
            landmark_pb2.NormalizedLandmark(x=landmark.x, y=landmark.y, z=landmark.z) for landmark in face_landmarks
        ])

        solutions.drawing_utils.draw_landmarks(
            image=annotated_image,
            landmark_list=face_landmarks_proto,
            connections=mp.solutions.face_mesh.FACEMESH_TESSELATION,
            landmark_drawing_spec=None,
            connection_drawing_spec=mp.solutions.drawing_styles.get_default_face_mesh_tesselation_style())
        solutions.drawing_utils.draw_landmarks(
            image=annotated_image,
            landmark_list=face_landmarks_proto,
            connections=mp.solutions.face_mesh.FACEMESH_CONTOURS,
            landmark_drawing_spec=None,
            connection_drawing_spec=mp.solutions.drawing_styles.get_default_face_mesh_contours_style())
        solutions.drawing_utils.draw_landmarks(
            image=annotated_image,
            landmark_list=face_landmarks_proto,
            connections=mp.solutions.face_mesh.FACEMESH_IRISES,
            landmark_drawing_spec=None,
            connection_drawing_spec=mp.solutions.drawing_styles.get_default_face_mesh_iris_connections_style())

    return annotated_image


if __name__ == "__main__":
    # 1 for right side, 2 for left side
    # 0 for no output video, 1 for output video
    main(1,0)