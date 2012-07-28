/*
 * main.cpp
 *
 * This file is part of the Simon Says project.
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

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <sstream>
#include <string.h>
#include <vector>
#include <map>
#include <math.h>
#include <time.h>
#include <iostream>
#include <dirent.h>

#include "globalConst.h"
#include "tHMM.h"
#include "tAgent.h"
#include "tGame.h"

#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */

#include "helper.h"           /*  our own helper functions  */

#define ECHO_PORT          (2002)
#define MAX_LINE           (100000)

#define BUFLEN              512
#define NPACK               10
#define PORT                9930

int       list_s;                /*  listening socket          */
int       conn_s;                /*  connection socket         */
short int port;                  /*  port number               */
struct    sockaddr_in servaddr;  /*  socket address structure  */
char      buffer[MAX_LINE];      /*  character buffer          */
char     *endptr;                /*  for strtol()              */

void    setupBroadcast(void);
void    doBroadcast(string data);

using namespace std;

//double  replacementRate             = 0.1;
double  perSiteMutationRate         = 0.005;
int     populationSize              = 100;
int     totalGenerations            = 252;
tGame   *game                       = NULL;

bool    track_best_brains           = false;
int     track_best_brains_frequency = 25;
bool    make_logic_table            = false;
bool    make_dot                    = false;

int main(int argc, char *argv[])
{
	vector<tAgent*> gameAgents, GANextGen;
	tAgent *gameAgent = NULL, *bestGameAgent = NULL;
	double gameAgentMaxFitness = 0.0;
    string LODFileName = "", gameGenomeFileName = "", inputGenomeFileName = "";
    string gameDotFileName = "", logicTableFileName = "";
    
    // initial object setup
    gameAgents.resize(populationSize);
	game = new tGame;
	gameAgent = new tAgent;
    
    // time-based seed by default. can change with command-line parameter.
    srand((unsigned int)time(NULL));
    
    for (int i = 1; i < argc; ++i)
    {
        // -e [out file name] [out file name]: evolve
        if (strcmp(argv[i], "-e") == 0 && (i + 2) < argc)
        {
            ++i;
            stringstream lodfn;
            lodfn << argv[i];
            LODFileName = lodfn.str();
            
            ++i;
            stringstream sgfn;
            sgfn << argv[i];
            gameGenomeFileName = sgfn.str();
        }
        
        // -s [int]: set seed
        else if (strcmp(argv[i], "-s") == 0 && (i + 1) < argc)
        {
            ++i;
            srand(atoi(argv[i]));
        }
        
        // -g [int]: set generations
        else if (strcmp(argv[i], "-g") == 0 && (i + 1) < argc)
        {
            ++i;
            
            // add 2 generations because we look at ancestor->ancestor as best agent at end of run
            totalGenerations = atoi(argv[i]);
            
            if (totalGenerations < 3)
            {
                cerr << "minimum number of generations permitted is 3." << endl;
                exit(0);
            }
        }
        
        // -t [int]: track best brains
        else if (strcmp(argv[i], "-t") == 0 && (i + 1) < argc)
        {
            track_best_brains = true;
            ++i;
            track_best_brains_frequency = atoi(argv[i]);
            
            if (track_best_brains_frequency < 1)
            {
                cerr << "minimum brain tracking frequency is 1." << endl;
                exit(0);
            }
        }
        
        // -lt [in file name] [out file name]: create logic table for given genome
        else if (strcmp(argv[i], "-lt") == 0 && (i + 2) < argc)
        {
            ++i;
            gameAgent->loadAgent(argv[i]);
            gameAgent->setupPhenotype();
            ++i;
            stringstream ltfn;
            ltfn << argv[i];
            logicTableFileName = ltfn.str();
            make_logic_table = true;
        }
        
        // -df [in file name] [out file name]: create dot image file for given genome
        else if (strcmp(argv[i], "-df") == 0 && (i + 2) < argc)
        {
            ++i;
            gameAgent->loadAgent(argv[i]);
            gameAgent->setupPhenotype();
            ++i;
            stringstream dfn;
            dfn << argv[i];
            gameDotFileName = dfn.str();
            make_dot = true;
        }
    }

    if (make_logic_table)
    {
        gameAgent->saveLogicTable(logicTableFileName.c_str());
        exit(0);
    }
    
    if (make_dot)
    {
        gameAgent->saveToDot(gameDotFileName.c_str());
        exit(0);
    }
    
    // seed the agents
    delete gameAgent;
    gameAgent = new tAgent;
    //gameAgent->setupRandomAgent(5000);
    gameAgent->loadAgent("gameAgent.genome");
    
    // make mutated copies of the start genome to fill up the initial population
	for(int i = 0; i < populationSize; ++i)
    {
		gameAgents[i] = new tAgent;
		gameAgents[i]->inherit(gameAgent, 0.01, 1);
    }
    
	GANextGen.resize(populationSize);
    
	gameAgent->nrPointingAtMe--;
    
	cout << "setup complete" << endl;
    cout << "starting evolution" << endl;
    
    // main loop
	for (int update = 1; update <= totalGenerations; ++update)
    {
        // reset fitnesses
		for(int i = 0; i < populationSize; ++i)
        {
			gameAgents[i]->fitness = 0.0;
			//gameAgents[i]->fitnesses.clear();
		}
        
        // determine fitness of population
		gameAgentMaxFitness = 0.0;
        double gameAgentAvgFitness = 0.0;
        
		for(int i = 0; i < populationSize; ++i)
        {
            game->executeGame(gameAgents[i], NULL, false);
            
            gameAgentAvgFitness += gameAgents[i]->fitness;
            
            //gameAgents[i]->fitnesses.push_back(gameAgents[i]->fitness);
            
            if(gameAgents[i]->fitness > gameAgentMaxFitness)
            {
                gameAgentMaxFitness = gameAgents[i]->fitness;
                bestGameAgent = gameAgents[i];
            }
		}
        
        gameAgentAvgFitness /= (double)populationSize;
		
        if (update % 1000 == 0)
        {
            cout << "generation " << update << ": game agent [" << gameAgentAvgFitness << " : " << gameAgentMaxFitness << "]" << endl;
        }
        
		for(int i = 0; i < populationSize; ++i)
		{
            // construct swarm agent population for the next generation
			tAgent *offspring = new tAgent;
            int j = 0;
            
			do
            {
                j = rand() % populationSize;
            } while((j == i) || (randDouble > (gameAgents[j]->fitness / gameAgentMaxFitness)));
            
			offspring->inherit(gameAgents[j], perSiteMutationRate, update);
			GANextGen[i] = offspring;
		}
        
		for(int i = 0; i < populationSize; ++i)
        {
            // retire and replace the game agents from the previous generation
			gameAgents[i]->retire();
			gameAgents[i]->nrPointingAtMe--;
			if(gameAgents[i]->nrPointingAtMe == 0)
            {
				delete gameAgents[i];
            }
			gameAgents[i] = GANextGen[i];
		}
        
		gameAgents = GANextGen;
        
        if (track_best_brains && update % track_best_brains_frequency == 0)
        {
            stringstream sss;
            
            sss << "gameAgent" << update << ".genome";
            
            gameAgents[0]->ancestor->ancestor->saveGenome(sss.str().c_str());
        }
	}
	
    // save the genome file of the lmrca
	bestGameAgent->saveGenome(gameGenomeFileName.c_str());
    
    // save quantitative stats on the best game agent's LOD
    /*vector<tAgent*> saveLOD;
    
    cout << "building ancestor list" << endl;
    
    // use 2 ancestors down from current population because that ancestor is highly likely to have high fitness
    tAgent* curAncestor = gameAgents[0]->ancestor->ancestor;
    
    while (curAncestor != NULL)
    {
        // don't add the base ancestor
        if (curAncestor->ancestor != NULL)
        {
            saveLOD.insert(saveLOD.begin(), curAncestor);
        }
        
        curAncestor = curAncestor->ancestor;
    }
    
    FILE *LOD = fopen(LODFileName.c_str(), "w");

    fprintf(LOD, "generation,prey_fitness,predator_fitness,num_alive_end,avg_bb_size,var_bb_size,avg_shortest_dist,swarm_density_count,prey_neurons_connected_prey_retina,prey_neurons_connected_predator_retina,predator_neurons_connected_prey_retina,mutual_info\n");
    
    cout << "analyzing ancestor list" << endl;
    
    for (vector<tAgent*>::iterator it = saveLOD.begin(); it != saveLOD.end(); ++it)
    {
        // collect quantitative stats
        game->executeGame(*it, LOD, false);
    }
    
    fclose(LOD);*/
    
    return 0;
}

void setupBroadcast(void)
{
    port = ECHO_PORT;
	if ( (list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
		fprintf(stderr, "ECHOSERV: Error creating listening socket.\n");
    }
	/*  Set all bytes in socket address structure to
	 zero, and fill in the relevant data members   */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(port);
	/*  Bind our socket addresss to the 
	 listening socket, and call listen()  */
    if ( bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 )
    {
		fprintf(stderr, "ECHOSERV: Error calling bind()\n");
    }
    if ( listen(list_s, LISTENQ) < 0 )
    {
		fprintf(stderr, "ECHOSERV: Error calling listen()\n");
    }
}

void doBroadcast(string data)
{
    if ( (conn_s = accept(list_s, NULL, NULL) ) < 0 )
    {
        fprintf(stderr, "ECHOSERV: Error calling accept()\n");
    }
    Writeline(conn_s, data.c_str(), data.length());
    
    if ( close(conn_s) < 0 )
    {
        fprintf(stderr, "ECHOSERV: Error calling close()\n");
    }
}
