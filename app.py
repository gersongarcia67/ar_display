import os, random
import requests
import logging
import sys
from flask import Flask
from flask import request
from flask import json
from flask import render_template
from jinja2 import Template,FileSystemLoader,Environment
import threading
import time as t

app = Flask(__name__)
app.config['PROPAGATE_EXCEPTIONS'] = True

@app.route("/", methods=["GET"])
def ar_ui():
    return render_template('index.html')

@app.route("/json", methods=["POST"])
def json_upload():

    req=request.get_json()
    json_req=json.dumps(req)

    with open("/data/esp8266.json",'w') as f:
        f.write(str(json_req))
    f.close()

    env=Environment(loader=FileSystemLoader('/templates'),)
    int_template=env.get_template('index.html.j2')


    trender=int_template.render(req)

    with open('/templates/index.html','w') as f:
        for line in trender.split('\n'):
           f.write('%s\n' % line)
    f.close()

    return("%s\n" % json_req)

# Workaround to run http and https same code

def runHttps():
    app.run(host="0.0.0.0", port=5051, ssl_context='adhoc', debug=True, use_reloader=False)

def runHttp():
    app.run(host="0.0.0.0", port=5050, debug=True, use_reloader=False)


if __name__ == "__main__":

    http = threading.Thread(target=runHttp)
    https = threading.Thread(target=runHttps)
    http.start()
    t.sleep(0.5)
    https.start()


