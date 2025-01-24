from flask import Flask,render_template, request, jsonify
from bci_essentials import *
import os
import csv
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
    time = request.json('time')
    id = request.json('id')
    timeID = [time, id]
    print (timeID)
    rows = [timeID]
    timestring = str(timeID[0]) + str(timeID[1])
    with open("output.csv", "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["Column 1", "Column 2"])  # Header row
        writer.writerows(rows)

    print("Data saved to output.csv")
    if os.path.exists("C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/test.txt"):
        f = open("C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/test.txt", "a")
    else:
        f = open("C:/Users/thepi/Documents/Capstone/neuromove/P300/arrowtesting/test.txt", 'w')
    f.writelines(timestring)
    f.close()
    return render_template('stop_go.html')

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
