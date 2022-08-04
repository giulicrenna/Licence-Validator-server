import json
import random as r
from secrets import choice

dict = {}
characters = ["A", "B", "C", "D", "E", "F", "G", "H",
                "I", "J", "K", "L", "M",
                "N", "O", "P", "Q", "R",
                "S", "T", "U", "V", "W",
                "X", "Y", "1", "2", "3",
                "4", "5", "6", "7", "8", "9"]
duet = ["A", "B", "C", "D", "E", "F", "G", "H",
                "I", "J", "K", "L", "M",
                "N", "O", "P", "Q"]

def generateLicence():
    licenceN = 0
    for char in duet:
        for i in range(500):
            licenceN += 1
            licence = "A" + char + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + "-" +  r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters)
            licenceName = "Licence_" + str(licenceN)
            dict[licenceName] = licence
    f = open("licences_Idented.json", "a")
    f.write(json.dumps(dict, indent=4))
    f.close()
    f = open("licences.json", "a")
    f.write(json.dumps(dict))
    f.close()



generateLicence()
print(json.dumps(dict, indent=4))