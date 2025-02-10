from flask import Flask,render_template, request, jsonify
from bci_essentials import *
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from ast import literal_eval
import logging
from queue import Queue
# from kmedoids import fasterpam
from scipy.spatial.distance import pdist, squareform, cdist
import threading


from pylsl import StreamInfo, StreamOutlet, local_clock
'''import os
import csv
import sqlite3 as sql'''

app = Flask(__name__,template_folder="templates")

# create LSL stream
marker_info = StreamInfo(name='MarkerStream',
                        type='LSL_Marker_Strings',
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

@app.route("/training")
def training():
    return render_template('training.html')
    
@app.route("/dotcoords", methods=['GET', 'POST'])
def dotcoords():
    data = request.json['data']
    file_path = 'middles.txt'
    with open(file_path, "r") as f:
        coords = f.readlines()
    return jsonify(coords)

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
    # file_path = "C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/test1.txt"
    file_path = "test1.txt"
    with open(file_path, "a") as f:  # Open in append mode
        f.write(str(timeID) + '\n')

    # format data sample 1
    if (len(timeID[1]) == 1):
        # for a square flashed, we need the special format
        flashed_as_num = ord(timeID[1]) - 98
        current_target = timeID[2]
        marker = f"p300,s,{NUMBER_OF_OPTIONS},{current_target},{flashed_as_num}"
    else:
        # for an "event" marker we just need the string
        marker = timeID[1]

    # add in lsl timestamp
    timestamp = float(timeID[0]) + local_clock()

    # broadcast to LSL
    marker_outlet.push_sample([marker], timestamp)

    return 1

@app.route('/eyetrackingside', methods=['POST'])
def eyetrackingside():
    data = request.json['data']
    eyeoutput(data)
    return jsonify({'result': 'success'})

def eyeoutput(val):
    file_path = "C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/test.txt"
    with open(file_path, "a") as f:  # Open in append mode
        f.write(str(val) + '\n')

def drawMap():
    data = np.loadtxt('data.txt')
    medoid_coordinates = np.loadtxt('middles.txt')
    neighbourhood_points = np.loadtxt('neighbourhood_points.txt').reshape((4,4,2))
    origin = np.loadtxt('origin.txt')
    number_of_neighbourhoods = neighbourhood_points.shape[0]
    t0 = threading.Thread(map0(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods)) 
    t1 = threading.Thread(map1(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods)) 
    t2 = threading.Thread(map2(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods)) 
    t3 = threading.Thread(map3(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods)) 
    t4 = threading.Thread(map4(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods)) 
    
    t0.start()
    t1.start()
    t2.start()
    t3.start()
    t4.start()

    t0.join()
    t1.join()
    t2.join()
    t3.join()
    t4.join()


    #base map
def map0(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods):
    
    colours = ['#b0b9cc', '#000000', '#000000', '#000000', '#000000']
    colourmap = ListedColormap(colours)
    plt.imshow(data, cmap=colourmap, interpolation='nearest')
    plt.gca().invert_yaxis()

    # save just colour zones
    plt.axis('off')
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    #plt.savefig('no-points.png', format='png', bbox_inches='tight', pad_inches=0)

    plt.scatter(origin[0], origin[1], color='red', marker='*')
    plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

    # save with origin and centers
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.savefig('static/center-points0.svg', format='svg', bbox_inches='tight', pad_inches=0)

    #map with region 1 flashed
def map1(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods):
    
    colours = ['#b0b9cc', '#FFFFFF', '#000000', '#000000', '#000000']
    colourmap = ListedColormap(colours)
    plt.imshow(data, cmap=colourmap, interpolation='nearest')
    plt.gca().invert_yaxis()

    # save just colour zones
    plt.axis('off')
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    #plt.savefig('no-points.png', format='png', bbox_inches='tight', pad_inches=0)

    plt.scatter(origin[0], origin[1], color='red', marker='*')
    plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

    # save with origin and centers
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.savefig('static/center-points1.svg', format='svg', bbox_inches='tight', pad_inches=0)

def map2(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods):
    #map with region 2 flashed
    
    colours = ['#b0b9cc', '#000000', '#FFFFFF', '#000000', '#000000']
    colourmap = ListedColormap(colours)
    plt.imshow(data, cmap=colourmap, interpolation='nearest')
    plt.gca().invert_yaxis()

    # save just colour zones
    plt.axis('off')
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    #plt.savefig('no-points.png', format='png', bbox_inches='tight', pad_inches=0)

    plt.scatter(origin[0], origin[1], color='red', marker='*')
    plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

    # save with origin and centers
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.savefig('static/center-points2.svg', format='svg', bbox_inches='tight', pad_inches=0)


    #map with region 3 flashed
def map3(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods):
    
    colours = ['#b0b9cc', '#000000', '#000000', '#FFFFFF', '#000000']
    colourmap = ListedColormap(colours)
    plt.imshow(data, cmap=colourmap, interpolation='nearest')
    plt.gca().invert_yaxis()

    # save just colour zones
    plt.axis('off')
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    #plt.savefig('no-points.png', format='png', bbox_inches='tight', pad_inches=0)

    plt.scatter(origin[0], origin[1], color='red', marker='*')
    plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

    # save with origin and centers
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.savefig('static/center-points3.svg', format='svg', bbox_inches='tight', pad_inches=0)


    #map with region 4 flashed
def map4(data, medoid_coordinates, neighbourhood_points, origin, number_of_neighbourhoods):
    
    colours = ['#b0b9cc', '#000000', '#000000', '#000000', '#FFFFFF']
    colourmap = ListedColormap(colours)
    plt.imshow(data, cmap=colourmap, interpolation='nearest')
    plt.gca().invert_yaxis()

    # save just colour zones
    plt.axis('off')
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    #plt.savefig('no-points.png', format='png', bbox_inches='tight', pad_inches=0)

    plt.scatter(origin[0], origin[1], color='red', marker='*')
    plt.scatter(medoid_coordinates[:, 1], medoid_coordinates[:, 0], color='black')

    # save with origin and centers
    plt.subplots_adjust(left=0, right=1, top=1, bottom=0)
    plt.savefig('static/center-points4.svg', format='svg', bbox_inches='tight', pad_inches=0)



if __name__ == '__main__':
    app.run(debug=True)
