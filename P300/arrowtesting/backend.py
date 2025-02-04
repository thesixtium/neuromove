from flask import Flask,render_template, request, jsonify
from bci_essentials import *

from pylsl import StreamInfo, StreamOutlet
'''import os
import csv
import sqlite3 as sql'''

app = Flask(__name__,template_folder="templates")

# create LSL stream
marker_info = StreamInfo(name='MarkerStream',
                        type='Markers',
                        channel_count=1,
                        nominal_srate=250,
                        channel_format='string',
                        source_id='Marker_Outlet')
marker_outlet = StreamOutlet(marker_info, 20, 360)  
NUMBER_OF_OPTIONS = 5

@app.route("/")
def hello():
    return render_template('destination.html')

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
    
@app.route("/dotcoords", methods=['GET', 'POST'])
def dotcoords():
    data = request.json['data']
    file_path = 'C:/Users/philippa.madill/Documents/GitHub/neuromove/P300/arrowtesting/static/centers.txt'
    with open(file_path, "r") as f:
        coords = f.readlines()
    dotarray = coords
    return jsonify(dotarray)

@app.route("/destination")
def destination():
    return render_template('destination.html')

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

    # format data sample 
    # TODO: get what is flashing (for training) or a flag saying we're not training
    current_target = -1
    flashed_as_num = ord(timeID[1]) - 97
    marker = f"p300,s,{NUMBER_OF_OPTIONS},{current_target},{flashed_as_num}"

    timestamp = float(timeID[0])

    # broadcast to LSL
    marker_outlet.push_sample([marker], timestamp)

    return 1

@app.route('/eyetrackingside', methods=['POST'])
def eyetrackingside():
    data = request.form.get('data')
    return 

if __name__ == '__main__':
    app.run(debug=True)
