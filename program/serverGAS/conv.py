import json
import os

outputDir="gasCode"

#https://qiita.com/soundTricker/items/8873f29781d1e123cfa8

if not os.path.exists(outputDir):
    os.makedirs(outputDir)

with open("picaBookmarker.json","r",encoding="utf-8") as f:
    data=json.load(f)
    print(data)
    for file in data["files"]:
        fileType=file['type']
        if fileType=="server_js":
            fileType="gs"
        name=f"{file['name']}.{fileType}"
        source=file['source']
        print(name)
        print(source)

        with open(os.path.join("gasCode",name),"w",encoding="utf-8") as out:
            out.write(source)