from flask import Flask,render_template, request, jsonify
from bci_essentials import *
'''import os
import csv
import sqlite3 as sql'''

app = Flask(__name__,template_folder="templates")

@app.route("/")
def hello():
    return render_template('bcisetup.html')

@app.route("/local")
def local():
    return render_template('p300Arrows.html')

@app.route("/screenside")
def screenside():
    return render_template('screenside.html')

@app.route("/localBCI", methods=['POST'])
def localBCI():
    data = request.json['data']
   # time = data.get('time')
   # idrt = data.get('id')
   # timeID = [time, idrt]
    #print(timeID)
    outputpls(data)
    return jsonify({'result': 'success'})
    
'''@app.route("/dotcoords", methods=['GET', 'POST'])
def dotcoords():
    file_path = "P300/arrowtesting/static/centers.txt"
    with open(file_path, "r") as f:
        coords = f.readlines()
        return jsonify({'result': coords})
'''

@app.route("/stop_go", methods=['GET'])
def stopBCI():
    return render_template('stop_go.html')


@app.route("/setup")
def setup():
    return render_template('setupmenu.html')

def outputpls(timeID):
    file_path = "P300/arrowtesting/test1.txt"
    with open(file_path, "a") as f:  # Open in append mode
        f.write(str(timeID) + '\n')
    return 1

@app.route('/eyetrackingside', methods=['POST'])
def eyetrackingside():
    data = request.form.get('data')
    return 

if __name__ == '__main__':
    app.run(debug=True)
