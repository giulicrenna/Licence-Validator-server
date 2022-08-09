import json
import random as r
from secrets import choice


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
    f = open("licences.txt", "a")
    for char in duet:
        for i in range(200):
            licenceN += 1
            licence = "A" + char + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + "-" +  r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + r.choice(characters) + "," 
            f.write(licence)
    f.close()



generateLicence()
