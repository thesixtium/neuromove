from flask import Flask,render_template, request, jsonify
from bci_essentials import *
import os
import csv
import sqlite3 as sql

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

@app.route("/localBCI", methods=['GET', 'POST'])
def localBCI():
    data = request.get_json()
    time = data.get('time')
    idrt = data.get('id')
    timeID = [time, idrt]
    print(timeID)
    
    file_path = "C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/test1.json"
    with open(file_path, "a") as f:  # Open in append mode
        f.write(str(timeID) + '\n')
    
    try: 
        con = sql.connect('shot_database.db')
        c = con.cursor()
        c.execute("CREATE TABLE IF NOT EXISTS shot_table (time, id)")
        c.execute("INSERT INTO shot_table (time, id) VALUES (?,?)", (time, idrt))
        con.commit()

    except: 
        print("an error occured")

    return jsonify(timeID)
@app.route("/stop_go", methods=['GET'])
def stopBCI():
    time = request.json('time')
    id = request.json('id')
    timeID = [time, id]
    print (timeID)
    return timeID


@app.route("/setup")
def setup():
    return render_template('setupmenu.html')

if __name__ == '__main__':
    app.run(debug=True)
