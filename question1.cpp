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
	int left_to_execute;
	bool completed;
};

int main(int argc, char *argv[]) {
	ifstream in(argv[1]);
	if (!in.is_open()) {
		cout << "Error: Could not open file\n" << endl;
		return false;
	}
	string line;
	struct Task temp = { 0,0,0,0,0,false };
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
		temp.left_to_execute = temp.execution_time;
		tasks.push_back(temp);
	}
	// Stores the simulation time
	string junk;
	in >> junk >> simulation_time;

	// Begin Scheduling RM
	ofstream out;
	out.open("output1.txt");
	bool task_occuring = false;
	bool task_reset = false;
	struct Task shortest = { -1,-1,32767,-1,-1,false };
	int j = 1;
	out << "***RM SCHEDULING***" << endl;
	for (int current_time = 0; current_time <= simulation_time; current_time++) {
		// Check if any task's periods have restarted.
		for (unsigned int i = 0; i < tasks.size(); i++) {
			if (current_time % tasks.at(i).period == 0) {
				tasks.at(i).completed = false;
				task_reset = true;
			}
		}
		if (task_reset || !task_occuring) {
			// Check for the shortest period task of those not completed
			for (unsigned int i = 0; i < tasks.size(); i++) {
				if (tasks.at(i).period < shortest.period && !tasks.at(i).completed) {
					shortest = tasks.at(i);
					j = 1;
				}
			}
			// There was a task that hasn't been completed, so start it
			if (shortest.period != 32767)
				task_occuring = true;
		}
		// See if task is running
		if (task_occuring) {
			// If not complete
			if (j <= shortest.execution_time)
				out << current_time << ": Task" << shortest.id << endl;
			// Else, reset for next task
			else {
				task_occuring = false;
				for (unsigned int i = 0; i < tasks.size(); i++) {
					if (tasks.at(i).id == shortest.id) {
						tasks.at(i).completed = true;
					}
				}
				shortest = { -1,-1,32767,-1,-1,false };
				j = 1;
				// Check for the shortest period task of those not completed
				for (unsigned int i = 0; i < tasks.size(); i++) {
					if (tasks.at(i).period < shortest.period && !tasks.at(i).completed) {
						shortest = tasks.at(i);
					}
				}
				// There was a task that hasn't been completed, so start it
				if (shortest.period != 32767) {
					task_occuring = true;
					out << current_time << ": Task" << shortest.id << endl;
				}
			}
		}
		// All tasks currently completed and free time
		if (!task_occuring) {
			out << current_time << ":" << endl;
		}
		j++;
		task_reset = false;
	}
	out.close();
}
