#%%
# Import needed packages
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import seaborn
import pickle

from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn.metrics import accuracy_score, confusion_matrix, precision_score

#%%
# Import the dataset
data = pd.read_excel('Haden_Test.xlsx')

# Split dataset into features and target
X_test = data.iloc[:, 0:17]  # Features
X_test.to_numpy()
y_test = data.iloc[:, 18]
y_test.to_numpy()

# View count of each class
y_test.value_counts()

#%%
#load the pre-trained model
with open('GeneralizedModel.pkl','rb') as file:
    model = pickle.load(file)

# Make predictions for the test set
y_pred_test = model.predict_proba(X_test)
custom_threshold = 0.43  # Set your desired threshold
prediction = (y_pred_test[:, 1] >= custom_threshold).astype(int)

#%%
# View accuracy score
a = accuracy_score(y_test, prediction)
print(a)
# View confusion matrix for test data and predictions
confusion_matrix(y_test, prediction)

#%%
# Get and reshape confusion matrix data
matrix = confusion_matrix(y_test, y_pred_test)
matrix = matrix.astype('float') / matrix.sum(axis=1)[:, np.newaxis]

# Build the plot
plt.figure(figsize=(16,7))
# sns.set(font_scale=1.4)
# sns.heatmap(matrix, annot=True, annot_kws={'size':10},
#             cmap=plt.cm.Greens, linewidths=0.2)

# Add labels to the plot
class_names = ['Looking at Screen', 'No Screen']
tick_marks = np.arange(len(class_names))
tick_marks2 = tick_marks + 0.5
plt.xticks(tick_marks, class_names, rotation=25)
plt.yticks(tick_marks2, class_names, rotation=0)
plt.xlabel('Predicted label')
plt.ylabel('True label')
plt.title('Confusion Matrix for Random Forest Model')
plt.show()
# %%
