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
#include <algorithm>

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
	int deferrable;
	int left_to_execute;
	int time_to_deadline;
	bool completed;
	int preemptions;
	int deadlines_missed;
};

struct ATask {
	int id;
	int arrival_time;
	int execution_time;
	bool completed;
	int left_to_execute;
	int time_completed;
};

bool compareByID(const Task &a, const Task &b)
{
	return a.id < b.id;
}

int main(int argc, char *argv[]) {
	ifstream in(argv[1]);
	if (!in.is_open()) {
		cout << "Error: Could not open file\n" << endl;
		return false;
	}
	string line;
	struct Task temp = { 0,0,0,0,0,0,0,false,0,0 };
	struct ATask atemp = { 0,0,0,0,0,0 };
	vector<Task> tasks;
	vector<ATask> atasks;
	int number_of_aperiodic_tasks = 0;
	int simulation_time;
	// Stores the number of tasks
	getline(in, line, '\n');
	int number_of_tasks = stoi(line);
	// Stores all the tasks in a vector
	for (int i = 0; i < number_of_tasks; i++) {
		getline(in, line, '\n');
		std::istringstream iss(line);
		iss >> temp.id >> temp.execution_time >> temp.period >> temp.deadline >> temp.deferrable;
		temp.left_to_execute = temp.execution_time;
		temp.time_to_deadline = temp.deadline;
		temp.completed = false;
		temp.preemptions = 0;
		temp.deadlines_missed = 0;
		tasks.push_back(temp);
	}
	// Stores the simulation time
	getline(in, line, '\n');
	getline(in, line, '\n');
	number_of_aperiodic_tasks = stoi(line);
	for (int i = 0; i < number_of_aperiodic_tasks; i++) {
		getline(in, line, '\n');
		std::istringstream iss(line);
		iss >> atemp.id >> atemp.arrival_time >> atemp.execution_time;
		atemp.completed = false;
		atemp.left_to_execute = atemp.execution_time;
		atemp.time_completed = -1;
		atasks.push_back(atemp);
	}
	string junk;
	in >> junk >> simulation_time;
	std::sort(tasks.begin(), tasks.end(), compareByID);


	/////////////////////////
	// Begin Scheduling RM //
	/////////////////////////
	ofstream out;
	out.open("output2.txt");
	bool task_occuring = false;
	bool task_reset = false;
	int total_preemptions = 0;
	int total_deadlines_missed = 0;
	int last_task_id = -1;
	struct Task shortest = { -1,-1,32767,-1,0,-1,32767,false,0,0 };
	struct ATask oldest = { -1, 32767, -1, false, -1, -1 };
	bool aperiodic_task_waiting = false;
	out << "***RM SCHEDULING***" << endl;
	for (int current_time = 0; current_time <= simulation_time; current_time++) {
		// Check if aperiodic queue has any tasks remaining
		for (unsigned int i = 0; i < atasks.size(); i++) {
			if (!atasks.at(i).completed && atasks.at(i).arrival_time <= current_time) {
				aperiodic_task_waiting = true;
				task_reset = true;
			}
		}
		// Check if any task not complete will miss deadline
		if(task_occuring) {
			for (unsigned int i = 0; i < tasks.size(); i++) {
				// Current time has reached deadline and not done executing
				if ( current_time % (tasks.at(i).deadline + (current_time / tasks.at(i).period)
					* tasks.at(i).period) == 0 && tasks.at(i).left_to_execute != 0) {
					if (!tasks.at(i).deferrable) {
						out << "TASK" << tasks.at(i).id << " MISSED DEADLINE" << endl;
						total_deadlines_missed++;
						tasks.at(i).left_to_execute = tasks.at(i).execution_time;
						tasks.at(i).deadlines_missed++;
					}
					tasks.at(i).completed = true;
					task_reset = true;
					task_occuring = false;
					shortest = { -1,-1,32767,-1,0,-1,32767,false,0,0 };
				}
			}
		}
		// Check if any task's periods have restarted.
		for (unsigned int i = 0; i < tasks.size(); i++) {
			if (current_time % tasks.at(i).period == 0) {
				tasks.at(i).completed = false;
				tasks.at(i).left_to_execute = tasks.at(i).execution_time;
				task_reset = true;
			}
		}
		// If a task was reset, it may have higher priority than what was already running
		// or nothing was running so see if anything needs run
		if (task_reset || !task_occuring) {
			// Check for the shortest period task of those not completed
			for (unsigned int i = 0; i < tasks.size(); i++) {
				// If there are aperiodic tasks waiting and the server has priority, make it shortest
				if (tasks.at(i).period < shortest.period && !tasks.at(i).completed && tasks.at(i).deadlines_missed < 1
					&& tasks.at(i).deferrable && aperiodic_task_waiting)
					shortest = tasks.at(i);
				else if (tasks.at(i).period < shortest.period && !tasks.at(i).completed && tasks.at(i).deadlines_missed < 1)
					shortest = tasks.at(i);
			}
			// There is a task that hasn't been completed, so start it
			if (shortest.period != 32767)
				task_occuring = true;
		}
		// See if task is running and not complete
		if (task_occuring && shortest.left_to_execute != 0) {
			////////////////////////////////////
			// Handle deferrable server queue //
			////////////////////////////////////
			if (shortest.deferrable) {
				oldest = { -1, 32767, -1, false, -1, -1 };
				// Find the aperiodic task that started the earliest
				for (unsigned int i = 0; i < atasks.size(); i++) {
					if (atasks.at(i).arrival_time < oldest.arrival_time && !atasks.at(i).completed)
						oldest = atasks.at(i);
				}
				// See if server interrupted a task meaning preemption occurred.
				if (last_task_id > 0 && task_reset && !tasks.at(last_task_id - 1).completed && last_task_id != shortest.id) {
					out << current_time << ": Aperiodic Task" << oldest.id << " Preempted Task" << last_task_id << endl;
					for (unsigned int i = 0; i < tasks.size(); i++) {
						if (tasks.at(i).id == shortest.id) {
							tasks.at(i).preemptions++;
						}
					}
					total_preemptions++;
				}
				else
					out << current_time << ": Aperiodic Task" << oldest.id << endl;
				last_task_id = shortest.id;
			}
			else {
				// See if current task isn't the same as last task ran and a task was reset
				// meaning potential preemption occurred.
				if (last_task_id > 0 && task_reset && !tasks.at(last_task_id - 1).completed && last_task_id != shortest.id) {
					out << current_time << ": Task" << shortest.id << " Preempted Task" << last_task_id << endl;
					for (unsigned int i = 0; i < tasks.size(); i++) {
						if (tasks.at(i).id == shortest.id) {
							tasks.at(i).preemptions++;
						}
					}
					total_preemptions++;
				}
				else
					out << current_time << ": Task" << shortest.id << endl;
				last_task_id = shortest.id;
			}
		}
		// All tasks currently completed and free time
		else if (!task_occuring) {
			out << current_time << ":" << endl;
		}
		if (shortest.deferrable) {
			// Decrement the time the server still has to run
			shortest.left_to_execute--;
			for (unsigned int i = 0; i < tasks.size(); i++) {
				if (tasks.at(i).id == shortest.id) {
					tasks.at(i).left_to_execute = shortest.left_to_execute;
				}
			}
			// Decrement the time the aperiodic task still needs to run
			oldest.left_to_execute--;
			for (unsigned int i = 0; i < atasks.size(); i++) {
				if (atasks.at(i).id == oldest.id) {
					atasks.at(i).left_to_execute = oldest.left_to_execute;
				}
			}
			// See if server that ran on this runthrough used all of budget
			if (task_occuring && shortest.left_to_execute == 0) {
				task_occuring = false;
				for (unsigned int i = 0; i < tasks.size(); i++) {
					if (tasks.at(i).id == shortest.id) {
						tasks.at(i).completed = true;
					}
				}
				shortest = { -1,-1,32767,-1,0,-1,32767,false,0,0 };
			}
			// See if aperiodic task that ran on this runthrough is completed
			if (task_occuring && oldest.left_to_execute == 0) {
				task_occuring = false;
				for (unsigned int i = 0; i < atasks.size(); i++) {
					if (atasks.at(i).id == oldest.id) {
						atasks.at(i).completed = true;
						atasks.at(i).time_completed = current_time;
					}
				}
				oldest = { -1, 32767, -1, false, -1, -1 };
				shortest = { -1,-1,32767,-1,0,-1,32767,false,0,0 };
			}
		}
		else {
			// Decrement the time the current task still needs to run
			shortest.left_to_execute--;
			for (unsigned int i = 0; i < tasks.size(); i++) {
				if (tasks.at(i).id == shortest.id) {
					tasks.at(i).left_to_execute = shortest.left_to_execute;
				}
			}
			// See if task that ran on this runthrough is completed
			if (task_occuring && shortest.left_to_execute == 0) {
				task_occuring = false;
				for (unsigned int i = 0; i < tasks.size(); i++) {
					if (tasks.at(i).id == shortest.id) {
						tasks.at(i).completed = true;
					}
				}
				shortest = { -1,-1,32767,-1,0,-1,32767,false,0,0 };
			}
			task_reset = false;
			aperiodic_task_waiting = false;
		}
	}
	// Calculate average response time
	int average_aperiodic_response_time = 0;
	for (unsigned int i = 0; i < atasks.size(); i++) {
		if (atasks.at(i).time_completed == -1)
			atasks.at(i).time_completed = simulation_time;
		average_aperiodic_response_time += atasks.at(i).time_completed - atasks.at(i).arrival_time;
	}
	average_aperiodic_response_time = average_aperiodic_response_time / atasks.size();
	// Print out summary
	out << endl << "RM SUMMARY" << endl << "Total Deadlines Missed: " << total_deadlines_missed << endl;
	for (unsigned int i = 0; i < tasks.size(); i++) {
		out << "Task" << tasks.at(i).id << " Deadlines Missed: " << tasks.at(i).deadlines_missed << endl;
	}
	out << "Average Aperiodic Response Time: " << average_aperiodic_response_time << endl;
	out << "Total Preemptions: " << total_preemptions << endl;
	for (unsigned int i = 0; i < tasks.size(); i++) {
		out << "Task" << tasks.at(i).id << " Preemptions: " << tasks.at(i).preemptions << endl;
	}

	//////////////////////////
	// Begin Scheduling EDF //
	//////////////////////////
	task_occuring = false;
	task_reset = false;
	total_preemptions = 0;
	total_deadlines_missed = 0;
	last_task_id = -1;
	shortest = { -1,-1,32767,-1,0,-1,32767,false,0,0 };
	// Reset preemption and deadlines missed count
	for (unsigned int i = 0; i < tasks.size(); i++) {
		tasks.at(i).preemptions = 0;
		tasks.at(i).deadlines_missed = 0;
	}
	out << endl << endl << "***EDF SCHEDULING***" << endl;
	for (int current_time = 0; current_time <= simulation_time; current_time++) {
		// Check if any task not complete will miss deadline
		if (task_occuring) {
			for (unsigned int i = 0; i < tasks.size(); i++) {
				if (current_time % (tasks.at(i).deadline + (current_time / tasks.at(i).period)
					* tasks.at(i).period) == 0 && tasks.at(i).left_to_execute != 0) {
					out << "TASK" << tasks.at(i).id << " MISSED DEADLINE" << endl;
					total_deadlines_missed++;
					tasks.at(i).left_to_execute = tasks.at(i).execution_time;
					tasks.at(i).deadlines_missed++;
					tasks.at(i).completed = true;
					task_reset = true;
					task_occuring = false;
					shortest = { -1,-1,32767,-1,0,-1,32767,false,0,0 };
				}
			}
		}
		// Check if any task's periods have restarted.
		for (unsigned int i = 0; i < tasks.size(); i++) {
			if (current_time % tasks.at(i).period == 0) {
				tasks.at(i).completed = false;
				tasks.at(i).left_to_execute = tasks.at(i).execution_time;
				tasks.at(i).time_to_deadline = tasks.at(i).deadline;
				task_reset = true;
			}
		}
		// If a task was reset, it may have higher priority than what was already running
		// or nothing was running so see if anything needs run
		if (task_reset || !task_occuring) {
			// Check for the task with the closest deadline of those not completed
			for (unsigned int i = 0; i < tasks.size(); i++) {
				if (tasks.at(i).time_to_deadline < shortest.time_to_deadline && !tasks.at(i).completed && tasks.at(i).deadlines_missed < 1) {
					shortest = tasks.at(i);
				}
			}
			// There is a task that hasn't been completed, so start it ***********************************************************************Period?
			if (shortest.period != 32767)
				task_occuring = true;
		}
		// See if task is running and not complete
		if (task_occuring && shortest.left_to_execute != 0) {
			// See if current task isn't the same as last task ran and a task was reset
			// meaning potential preemption occurred.
			if (last_task_id > 0 && task_reset && !tasks.at(last_task_id - 1).completed && last_task_id != shortest.id) {
				out << current_time << ": Task" << shortest.id << " Preempted Task" << last_task_id << endl;
				for (unsigned int i = 0; i < tasks.size(); i++) {
					if (tasks.at(i).id == shortest.id) {
						tasks.at(i).preemptions++;
					}
				}
				total_preemptions++;
			}
			else
				out << current_time << ": Task" << shortest.id << endl;
			last_task_id = shortest.id;
		}
		// All tasks currently completed and free time
		else if (!task_occuring) {
			out << current_time << ":" << endl;
		}
		// Decrement the time the current task still needs to run
		shortest.left_to_execute--;
		for (unsigned int i = 0; i < tasks.size(); i++) {
			if (tasks.at(i).id == shortest.id) {
				tasks.at(i).left_to_execute = shortest.left_to_execute;
			}
		}
		// Decrement the time to deadline for all uncompleted tasks
		for (unsigned int i = 0; i < tasks.size(); i++) {
			if (!tasks.at(i).completed)
				if (current_time % tasks.at(i).deadline == 0)
					tasks.at(i).time_to_deadline = tasks.at(i).deadline;
				else
					tasks.at(i).time_to_deadline = (tasks.at(i).deadline + (current_time / tasks.at(i).period) * tasks.at(i).period) - current_time;
		}
		// See if task that ran on this runthrough is completed
		if (task_occuring && shortest.left_to_execute == 0) {
			task_occuring = false;
			for (unsigned int i = 0; i < tasks.size(); i++) {
				if (tasks.at(i).id == shortest.id) {
					tasks.at(i).completed = true;
				}
			}
			shortest = { -1,-1,32767,-1,0,-1,32767,false,0,0 };
		}
		task_reset = false;
	}
	// Print out summary
	out << endl << "EDF SUMMARY" << endl << "Total Deadlines Missed: " << total_deadlines_missed << endl;
	for (unsigned int i = 0; i < tasks.size(); i++) {
		out << "Task" << tasks.at(i).id << " Deadlines Missed: " << tasks.at(i).deadlines_missed << endl;
	}
	out << "Total Preemptions: " << total_preemptions << endl;
	for (unsigned int i = 0; i < tasks.size(); i++) {
		out << "Task" << tasks.at(i).id << " Preemptions: " << tasks.at(i).preemptions << endl;
	}
	out.close();
}