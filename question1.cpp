// ECE 4550 Project 1
// Generates a schedule for RM and EDF. Reads from an input file: # of tasks,
// 		simulation time, task ID, task execution time, task period (with implicit 
// 		deadline.
// To an output file, your program will show the corresponding RM and EDF schedules, 
//		clearly indicating when a job is being preempted and when a job misses its 
//		deadline. Jobs that miss their deadlines should be dropped at their respective
//		deadlines.

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::stoi;
using std::ifstream;
using std::ofstream;

struct Task{
	int id;
	int execution_time;
	int period;
	int deadline;
	bool completed;
};

int main(int argc, char *argv[]) {
	ifstream in(argv[1]);
	if (!in.is_open()) {
		cout << "Error: Could not open file\n" << endl;
		return false;
	}
	string line;
	struct Task temp = { 0,0,0,0,0 };
	vector<Task> tasks;
	int simulation_time;
	// Stores the number of tasks
	getline(in, line, '\n');
	int number_of_tasks = stoi(line);
	// Stores all the tasks in a vector
	for(int i = 0; i < number_of_tasks; i++) {
		getline(in, line, '\n');
		std::istringstream iss(line);
		iss >> temp.id >> temp.execution_time >> temp.period >> temp.deadline;
		tasks.push_back(temp);
	}
	// Stores the simulation time
	string junk;
	in >> junk >> simulation_time;

	// Begin Scheduling RM
	ofstream out;
	out.open("output1.txt");
	bool task_occuring = false;
	struct Task shortest = { -1,-1,32767,-1,-1 };
	int j = 1;
	out << "***RM SCHEDULING***" << endl;
	for (int current_time = 1; current_time <= simulation_time; current_time++) {
		// If a task isn't occuring, find the shortest one that hasn't run
		if (!task_occuring) {
			for (unsigned int i = 0; i < tasks.size(); i++) {
				if (tasks.at(i).period < shortest.period && !tasks.at(i).completed) {
					shortest = tasks.at(i);
				}
			}
			if (shortest.period != 32767)
				task_occuring = true;
		}
		if (task_occuring && j <= shortest.execution_time) {
			out << current_time << ": Task" << shortest.id << endl;
		}
		// Reset for next task 
		else {
			task_occuring = false;
			for (unsigned int i = 0; i < tasks.size(); i++) {
				if (tasks.at(i).id == shortest.id) {
					tasks.at(i).completed = true;
				}
			}
			shortest = { -1,-1,32767,-1,-1 };
			j = 1;
		}
		j++;
	}
	out.close();
}

