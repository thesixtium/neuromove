from flask import Flask,render_template, request, jsonify

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

@app.route("/localBCI", methods=['GET'])
def localBCI():
    time = request.json('time')
    id = request.json('id')
    timeID = [time, id]
    print (timeID)
    return timeID

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
