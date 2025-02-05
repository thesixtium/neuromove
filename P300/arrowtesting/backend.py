from flask import Flask,render_template, request, jsonify
from bci_essentials import *
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from ast import literal_eval
import logging
from queue import Queue
from kmedoids import fasterpam
from scipy.spatial.distance import pdist, squareform, cdist


from pylsl import StreamInfo, StreamOutlet, local_clock
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
    
@app.route("/dotcoords", methods=['GET', 'POST'])
def dotcoords():
    data = request.json['data']
    file_path = 'C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/static/centers.txt'
    with open(file_path, "r") as f:
        coords = f.readlines()
    dotarray = [[14,12],[5,17],[17,26],[6,5],[5,29]]
    return jsonify(dotarray)

@app.route("/destination")
def destination():
    drawMap()
    return render_template('destination.html')

@app.route("/stop_go", methods=['GET'])
def stopBCI():
    return render_template('stop_go.html')


@app.route("/setup")
def setup():
    return render_template('setupmenu.html')

def outputpls(timeID):
    file_path = "C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/test1.txt"
    with open(file_path, "a") as f:  # Open in append mode
        f.write(str(timeID) + '\n')

    # format data sample 
    # TODO: get what is flashing (for training) or a flag saying we're not training
    current_target = -1
    flashed_as_num = ord(timeID[1]) - 98
    marker = f"p300,s,{NUMBER_OF_OPTIONS},{current_target},{flashed_as_num}"

    # convert from microseconds to seconds
    #timestamp = float(timeID[0]) / (10 ** 6) + local_clock()

    # broadcast to LSL
    marker_outlet.push_sample([marker], timeID[0])

    return 1

@app.route('/eyetrackingside', methods=['POST'])
def eyetrackingside():
    data = request.form.get('data')
    return 

def drawMap():
    data = np.loadtxt('data.txt')
    medoid_coordinates = np.loadtxt('middles.txt')
    neighbourhood_points = np.loadtxt('neighbourhood_points.txt').reshape((4,4,2))
    origin = np.loadtxt('origin.txt')

    number_of_neighbourhoods = neighbourhood_points.shape[0]

    colours = ['#202020', '#F5B528', '#FE6100', '#DC267F', '#648FFF']
    colourmap = ListedColormap(colours)
    plt.imshow(data, cmap=colourmap, interpolation='nearest')
    plt.gca().invert_yaxis()

    # save just colour zones
    plt.axis('off')
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.savefig('no-points.png', format='png', bbox_inches='tight', pad_inches=0)

    plt.scatter(origin[0], origin[1], color='red')
    plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

    # save with origin and centers
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.savefig('center-points.png', format='png', bbox_inches='tight', pad_inches=0)

    dark_colours = ['#A37104', '#7E3101', '#75013A', '#42367C']

    for i in range(number_of_neighbourhoods):
        plt.scatter(neighbourhood_points[i][:, 1], neighbourhood_points[i][:, 0], color=dark_colours[i])

    # replot medoids to make sure they're on top
    plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

    # save with all points
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.savefig('all-points.png', format='png', bbox_inches='tight', pad_inches=0)

    plt.axis('on')
    
    # Remove axis labels and whitespace
    #plt.show()
    plt.savefig("static/map.svg")


if __name__ == '__main__':
    app.run(debug=True)
