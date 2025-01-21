from flask import Flask,render_template, request, jsonify

app = Flask(__name__,template_folder="templates")

@app.route("/")
def hello():
    return render_template('setupmenu.html')

@app.route("/local")
def local():
    return render_template('p300Arrows.html')

@app.route('/process', methods=['POST'])
def process():
    data = request.get_json() # retrieve the data sent from JavaScript
    # process the data using Python code
    result = data['value'] * 2
    return jsonify(result=result) # return the result to JavaScript

if __name__ == '__main__':
    app.run(debug=True)
