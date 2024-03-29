/*
 * tAgent.h
 *
 * This file is part of the Simon Memory Game project.
 *
 * Copyright 2012 Randal S. Olson.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _tAgent_h_included_
#define _tAgent_h_included_

#include "globalConst.h"
#include "tHMM.h"
#include <vector>

using namespace std;

static int masterID = 0;

class tDot{
public:
	double xPos,yPos;
};


class tAgent{
public:
	vector<tHMMU*> hmmus;
	vector<unsigned char> genome;
	vector<tDot> dots;
    unsigned char nodeMap[256];
#ifdef useANN
	tANN *ANN;
#endif
	
	tAgent *ancestor;
	unsigned int nrPointingAtMe;
	unsigned char states[maxNodes],newStates[maxNodes];
	double fitness,convFitness;
	vector<double> fitnesses;
	int food;
	
	double xPos,yPos,direction;
	double sX,sY;
	bool foodAlreadyFound;
	int steps,bestSteps,totalSteps;
	int ID,nrOfOffspring;
	bool saved;
	bool retired;
	int born;
	int correct,incorrect;
	
	tAgent();
	~tAgent();
	void setupRandomAgent(int nucleotides);
    virtual void setupNodeMap(void);
	void loadAgent(char* filename);
	void loadAgentWithTrailer(char* filename);
	void setupPhenotype(void);
    void setupMegaPhenotype(int howMany);
	void inherit(tAgent *from,double mutationRate,double duplicationRate,double deletionRate,int theTime);
	unsigned char * getStatesPointer(void);
	void updateStates(void);
	void resetBrain(void);
	void ampUpStartCodons(void);
	void showBrain(void);
	void showPhenotype(void);
	void saveToDot(const char *filename);
	void saveToDotFullLayout(char *filename);
	
	void initialize(int x, int y, int d);
	tAgent* findLMRCA(void);
	void saveFromLMRCAtoNULL(FILE *statsFile,FILE *genomeFile);
	//void saveLOD(FILE *statsFile,FILE *genomeFile);
	void retire(void);
	void setupDots(int x, int y,double spacing);
	void saveLogicTable(const char *filename);
	void saveGenome(const char *filename);
};

#endif