import pickle
import joblib

# with open('GeneralizedModel2.pkl','rb') as file:
#     model = pickle.load(file)

# for i in range(10):
#     print(i)
#     joblib.dump(model, f'GeneralizedModel2Edit{i}', compress=i)

model = joblib.load('GeneralizedModel2Edit3')
with open('GeneralizedModelEdit5.pkl','wb') as file:
    pickle.dump(model, file)