# Script : potholeAmalgamation.py
# Author : Liam Gowan
# Date   : May 6th, 2019
# Purpose: This script will take two CSV's, one containing a master list of 
#		   potholes, and the other containing data to be added. The data to 
#		   be read in is in the format of {latitude, longitude, date, time, 
#		   number of satellites, bump measurement, course, and if it's verified}.
#
#          The resulting csv is in the form {ID, latitude, longitude, first date, 
#		   last date, bump measurement, course, # of occurences, and if it's verified}
#
#          Bump measurement and course are averaged. The number of occurences only 
#		   increases for unverified occurences.
#
#          The "master.csv" contains a list of all amalgamated potholes.
#
# References:
# Haversine formula (see response by Michael Dunn)
# https://stackoverflow.com/questions/4913349/haversine-formula-in-python-bearing-and-distance-between-two-gps-points
# 
# Reading/writing CSV:
# https://realpython.com/python-csv/
# 

import csv
import sys 
from math import radians, cos, sin, asin, sqrt
from datetime import datetime

# Variables to control the buffer of acceptance for additional potholes, but only 
# if they are within the specified course difference.
# (This is to prevent potholes on opposite sides of the road, but close to each 
# other from being considered as the same one)
metreDifference = 10.0
courseDifference = 20.0

#Lists to hold information regarding potholes to be amalgamated 
dataLat = []
dataLong = []
dataDate = []
dataBump = []
dataCourse = []
dataIsVerified = []

#Lists to hold information regarding already known potholes
masterID = []
masterLat = []
masterLong = []
masterFirstDate = []
masterLastDate = []
masterBump = []
masterCourse = []
masterOccurence = []
masterIsVerified = []

#Function will read in the data to be amalgamated and fill all the relevant lists. 
#It will check that there is data, if not, exits
def loadData(fileName):
    try:
        with open(fileName, 'rU') as dataFile:
            dataReader = csv.reader(dataFile)
            for row in dataReader:
                dataLat.append(float(row[0]))
                dataLong.append(float(row[1]))
                dataDate.append(row[2])
                dataBump.append(int(row[5]))
                dataCourse.append(float(row[6]))
                dataIsVerified.append(row[7])
            dataFile.close()
        if(len(dataLat)==0):
            print("Data file is empty. Exiting.")
            sys.exit()
    except:
        print "Error with provided file. Ensure file is named properly, is a csv, and data is in correct form.\n\tExiting."
        sys.exit()

#Function will read in data from the master csv. 
def loadMaster():
    with open('master.csv', 'rU') as masterFile:
        masterReader = csv.reader(masterFile)
        next(masterReader)
        for row in masterReader:
            masterID.append(int(row[0]))
            masterLat.append(float(row[1]))
            masterLong.append(float(row[2]))
            masterFirstDate.append(row[3])
            masterLastDate.append((row[4]))
            masterBump.append(float(row[5]))
            masterCourse.append(float(row[6]))
            masterOccurence.append(int(row[7]))
            masterIsVerified.append(row[8])
    masterFile.close()

    #If this is empty, then add the first line of the data to be added
    if(len(masterLat)==0):
        with open('master.csv', 'ab') as writeFile:
            writer = csv.writer(writeFile)
            row = [str(1),str(dataLat[0]),str(dataLong[0]),str(dataDate[0]),str(dataDate[0]),str(dataBump[0]),str(dataCourse[0]),str(0), str(dataIsVerified[0])]
            masterID.append(1)
            masterLat.append(dataLat[0])
            masterLong.append(dataLong[0])
            masterFirstDate.append(dataDate[0])
            masterLastDate.append(dataDate[0])
            masterBump.append(dataBump[0])
            masterCourse.append(dataCourse[0])
            masterOccurence.append(0)                   #assume to be zero for now, will update later if required
            masterIsVerified.append(dataIsVerified[0])
            writer.writerow(row)
        writeFile.close()

#Function will amalgamate the potholes, either updating existing entries or adding new ones
def amalgamate():
    lenData = len(dataLat)
    lenMaster = len(masterID)
    newRecordCount  = 0
    existingModifiedCount = 0
    
    #outer loop handles data from the data to be read in
    for i in range(lenData):
        toWrite = True
        #inner loop handles data already known
        for j in range(len(masterID)):
            diff = haversine(dataLat[i],dataLong[i], masterLat[j], masterLong[j])
            #If within buffer distance and course difference, then modify existing record
            if(diff<(metreDifference/1000) and dataCourse[i] >= (masterCourse[j] - courseDifference) and dataCourse[i] <= (masterCourse[j] + courseDifference)):
                toWrite = False
                #Determine changes to make to existing record
                with open('master.csv','rU') as readFile:
                    reader = csv.reader(readFile)
					#Determine bump
                    bumpToWrite = getBumpAmount(dataBump[i], masterBump[j])  
					#Get average course					
                    averageCourse = (dataCourse[i]+masterCourse[j])/2.0   
					#Get earliest date					
                    firstDate = getFirstDate(dataDate[i], masterFirstDate[j], masterLastDate[j]) 
					#Get most recent date					
                    lastDate = getLastDate(dataDate[i], masterFirstDate[j], masterLastDate[j]) 
					#Determines if occurrence is increased					
                    occurenceValue = masterOccurence[j] + determineOccurence(dataIsVerified[i])  
					#Determines verification status					
                    isVerified = getVerificationStatus(dataIsVerified[i], masterIsVerified[j])      
                    #Sets all lines to existing ones, except the one to be modified
                    lines = list(reader)
                    lines[j+1] = [str(masterID[j]),str(masterLat[j]),str(masterLong[j]),str(firstDate),str(lastDate),str(bumpToWrite),str(averageCourse),str(occurenceValue),str(isVerified)]

                    #Updates specified record in memory
                    masterID[j] = masterID[j]
                    masterLat[j] = masterLat[j]
                    masterLong[j] = masterLong[j]
                    masterFirstDate[j] =  firstDate
                    masterLastDate[j] = lastDate
                    masterBump[j] = bumpToWrite
                    masterCourse[j] = averageCourse
                    masterOccurence[j] = occurenceValue
                    masterIsVerified[j] = isVerified
                readFile.close()

                #Edit master.csv to have changes
                with open('master.csv', 'wb') as writeFile:
                    writer = csv.writer(writeFile)
                    writer.writerows(lines)
                writeFile.close()

                #increase count of records modified
                existingModifiedCount = existingModifiedCount + 1

        #If there is no existing match, this adds a new record
        if toWrite==True:
            with open('master.csv', 'ab') as writeFile:
                writer = csv.writer(writeFile)
				#Determine if another occurrence should be added
                occurenceValue = determineOccurence(dataIsVerified[i]) 
				#Determines pothole ID				
                potholeID = len(masterID) + 1                           
                #Set up the row to be added
                row = [str(potholeID), str(dataLat[i]),str(dataLong[i]),str(dataDate[i]),str(dataDate[i]),str(dataBump[i]),str(dataCourse[i]), str(occurenceValue), str(dataIsVerified[i])]

                #Add all information of record in memory
                masterID.append(potholeID)
                masterLat.append(dataLat[i])
                masterLong.append(dataLong[i])
                masterFirstDate.append(dataDate[i])
                masterLastDate.append(dataDate[i])
                masterBump.append(dataBump[i])
                masterCourse.append(dataCourse[i])
                masterOccurence.append(occurenceValue)
                masterIsVerified.append(dataIsVerified[i])

                #Adds the row
                writer.writerow(row)
            writeFile.close()
            #increase count of records added
            newRecordCount = newRecordCount +1
    return [newRecordCount, existingModifiedCount]
                
#Function performs the haversine formula to calculate and return distance in metres between two points
def haversine(lat1, long1, lat2, long2):
    lat1, long1, lat2, long2 = map(radians, [lat1, long1, lat2, long2])

    deltaLat  = lat2 - lat1
    deltaLong = long2 - long1

    a = sin(deltaLat/2)**2 + cos(lat1) * cos(lat2) * sin(deltaLong/2)**2
    c = 2 * asin(sqrt(a)) 
    r = 6371 
    return c * r

#Function returns the earliest of three dates
def getFirstDate(date1Str, date2Str, date3Str):
    date1 = datetime.strptime(date1Str, '%m/%d/%Y')
    date2 = datetime.strptime(date2Str, '%m/%d/%Y')
    date3 = datetime.strptime(date3Str, '%m/%d/%Y')
    return min([date1,date2,date3]).strftime('%m/%d/%Y')

#Function returns the most recent of three dates
def getLastDate(date1Str, date2Str, date3Str):
    date1 = datetime.strptime(date1Str, '%m/%d/%Y')
    date2 = datetime.strptime(date2Str, '%m/%d/%Y')
    date3 = datetime.strptime(date3Str, '%m/%d/%Y')
    return max([date1,date2,date3]).strftime('%m/%d/%Y')

#Function returns verification status given two statuses. Only returns 'N' if both statuses are also 'N', otherwise returns 'Y'
def getVerificationStatus(status1, status2):
    if(status1=="N" and status2=="N"):
        return "N"
    else:
        return "Y"

#Function determines if another occurence should be added based off the status
def determineOccurence(status):
    if status=="N":
        return 1
    else:
        return 0

#Function determines the bump amount of a given bump measurement for aggregated data
def getBumpAmount(bump1, bump2):
    if(bump1 == -1 and bump2 >= 0):
        return bump2
    elif(bump2 == -1 and bump1 >= 0):
        return bump1
    elif(bump1 >=0 and bump2 >=0):
        return (bump1 + bump2)/2.0
    else:
        return -1
    
#Function continues to prompt user to enter files and if they wish to process more files, 
#so that multiple files can be read in during operation of program
def main():
    #Get file name, load data to add and master data in, amalgamate 
    fileName = raw_input("Enter CSV filename: ")
    loadData(fileName)
    loadMaster()
    results = amalgamate()

    #Print report of records added, modified and total
    print("New records added        : %5d" % results[0])
    print("Existing records modified: %5d" % results[1])
    print("Current total records    : %5d" % len(masterID))

    print("Thank you for using the system. Exiting.")
    sys.exit()
        
main()