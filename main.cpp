#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>

using namespace std;

enum task_type { invalid = 0, local, cloud };

int core_energy[3] = { 1, 2, 4 };

#define NUM_TASK 10
#define NUM_CORE 3

struct task {
	int id;
	int f_local;
	int f_receive;
	int f_send;
	int f_cloud;
	int r_local;
	int r_cloud;
	int core;
	double priority;
	int energy;
	task_type type;
	std::vector<int> predecessor;
	std::vector<int> successor;
	std::vector<int> runtime;
};

bool compare_priority(task a, task b) {
	return a.priority > b.priority;
}

template <typename T>
inline auto find_average(std::vector<T> v) {
	return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

template <typename T>
inline auto find_min(std::vector<T> v) {
	return v.empty() ? 0 : *std::min_element(v.begin(), v.end());
}

template <typename T>
inline auto find_max(std::vector<T> v) {
	return v.empty() ? 0 : *std::max_element(v.begin(), v.end());
}

template <typename T>
inline auto find_argmin(std::vector<T> v) {
	return std::distance(v.begin(), std::min_element(v.begin(), v.end()));
}

template <typename T>
inline auto find_argmax(std::vector<T> v) {
	return std::distance(v.begin(), std::max_element(v.begin(), v.end()));
}

void loadInput(string filename,vector<task>& tasks) {
	/*
	input format:
	taskID NUM_Pre NUM_Suc
	list of pre---
	list of suc---
	cost on core 1 2 3
	*/

	std::ifstream file(filename);

	for (int i = 0; i < NUM_TASK; i++) {
		task t;
		int id, p, s;
		file >> id;
		t.id = id;
		file >> p >> s;
		for (int i = 0; i < p; i++) {
			int temp;
			file >> temp;
			t.predecessor.push_back(temp);
		}
		for (int i = 0; i < s; i++) {
			int temp;
			file >> temp;
			t.successor.push_back(temp);
		}
		for (int i = 0; i < 3; i++) {
			int temp;
			file >> temp;
			t.runtime.push_back(temp);
		}
		t.core = 0;
		t.energy = 0;
		t.priority = 0.0;
		if (t.runtime[0] > 5 && t.runtime[1] > 5 && t.runtime[2] > 5) {
			t.type = cloud;
			t.f_local = 0;
			t.f_receive = 1000;
			t.f_send = 1000;
			t.f_cloud = 1000;
		}
		else {
			t.type = local;
			t.f_receive = 0;
			t.f_send = 0;
			t.f_cloud = 0;
			t.f_local = 0;
		}
		tasks.push_back(t);
	}

}

void prioritizing(vector<task>& tasks) {
	for (int i = tasks.size() - 1; i >= 0; i--) {
		vector<double> successor_priorities;
		for (auto iter : tasks[i].successor) {
			successor_priorities.push_back(tasks[iter].priority);
		}
		double w;
		if (tasks[i].type == local) {
			w = ((double)tasks[i].runtime[0]+ (double)tasks[i].runtime[1]+ (double)tasks[i].runtime[2])/3.0;
		}
		else {
			w = 5;
		}
		double temp = 0;
		for (int i = 0; i < successor_priorities.size(); i++) {
			if (successor_priorities[i] > temp) temp = successor_priorities[i];
		}
		tasks[i].priority = w + temp;
	}
}

vector<int> sort(vector<task> tasks) {
	stable_sort(tasks.begin(), tasks.end(), compare_priority);
	vector<int> task_priority_list;
	for (auto it : tasks) {
		task_priority_list.push_back(it.id);
	}
	return task_priority_list;
}

void unitSelection(vector<task>& tasks,vector<int> Plist) {
	vector<int> availableTime(6, 0);  //core0 core1 core2 send cloud receive
	tasks[Plist[0]].f_cloud = 0;
	tasks[Plist[0]].f_receive = 0;
	tasks[Plist[0]].f_send = 0;
	tasks[Plist[0]].f_local = find_min(tasks[Plist[0]].runtime);
	tasks[Plist[0]].core = find_argmin(tasks[Plist[0]].runtime);
	tasks[Plist[0]].r_local = 0;
	tasks[Plist[0]].r_cloud = 0;
	tasks[Plist[0]].type = local;

	availableTime[tasks[Plist[0]].core] =tasks[Plist[0]].f_local;

	for(int i=1;i<NUM_TASK;i++){
		int current = Plist[i];
		if (tasks[current].type == local) {
			vector<int> predecessorFinishTime;
			for (auto x : tasks[current].predecessor) {
				if (tasks[x].type == local) {
					predecessorFinishTime.push_back(tasks[x].f_local);
				}
				else if (tasks[x].type == cloud) {
					predecessorFinishTime.push_back(tasks[x].f_receive);
				}
			}
			tasks[current].r_local = find_max(predecessorFinishTime);
			//
			vector<int>finishTime;
			for (int coreId = 0; coreId < 3; coreId++) {
				finishTime.push_back(std::max(availableTime[coreId],tasks[current].r_local) +tasks[current].runtime[coreId]);
			}
			tasks[current].f_local = find_min(finishTime);
			tasks[current].core = find_argmin(finishTime);
			tasks[current].r_local =tasks[current].f_local -tasks[current].runtime[tasks[current].core];
			availableTime[tasks[current].core] =tasks[current].f_local;
		}
		else {
			std::vector<int> predecessor_ft_ws;
			for (auto it : tasks[current].predecessor) {
				if (tasks[it].type == local) {
					predecessor_ft_ws.push_back(tasks[it].f_local);
				}
				else if (tasks[it].type == cloud) {
					predecessor_ft_ws.push_back(tasks[it].f_send);
				}
			}
			std::vector<int> predecessor_ft_c;
			for (auto it : tasks[current].predecessor) {
				if (tasks[it].type == local) {
					predecessor_ft_c.push_back(0);
				}
				else if (tasks[it].type == cloud) {
					predecessor_ft_c.push_back(tasks[it].f_cloud);
				}
			}
			tasks[current].r_cloud =std::max(find_max(predecessor_ft_ws), find_max(predecessor_ft_c));
			tasks[current].r_cloud =std::max(availableTime[3],tasks[current].r_cloud);
			tasks[current].f_send =tasks[current].r_cloud + 3;
			tasks[current].f_cloud =tasks[current].f_send + 1;
			tasks[current].f_receive =tasks[current].f_cloud + 1;
			availableTime[3] =tasks[current].f_send;


		}
	
	
	}
}

void recalculate(vector<task>& tasks, vector<int> Plist, vector<int> availableTime) {
	
	for (int i = 0; i < Plist.size(); i++) {
		int current = Plist[i];
		if (tasks[current].type == local) {
			vector<int> predecessorFinishTime;
			for (auto x : tasks[current].predecessor) {
				if (tasks[x].type == local) {
					predecessorFinishTime.push_back(tasks[x].f_local);
				}
				else if (tasks[x].type == cloud) {
					predecessorFinishTime.push_back(tasks[x].f_receive);
				}
			}
			tasks[current].r_local = find_max(predecessorFinishTime);
			//
			vector<int>finishTime;
			for (int coreId = 0; coreId < 3; coreId++) {
				finishTime.push_back(std::max(availableTime[coreId], tasks[current].r_local) + tasks[current].runtime[coreId]);
			}
			tasks[current].f_local = find_min(finishTime);
			tasks[current].core = find_argmin(finishTime);
			tasks[current].r_local = tasks[current].f_local - tasks[current].runtime[tasks[current].core];
			availableTime[tasks[current].core] = tasks[current].f_local;
		}
		else {
			std::vector<int> predecessor_ft_ws;
			for (auto it : tasks[current].predecessor) {
				if (tasks[it].type == local) {
					predecessor_ft_ws.push_back(tasks[it].f_local);
				}
				else if (tasks[it].type == cloud) {
					predecessor_ft_ws.push_back(tasks[it].f_send);
				}
			}
			std::vector<int> predecessor_ft_c;
			for (auto it : tasks[current].predecessor) {
				if (tasks[it].type == local) {
					predecessor_ft_c.push_back(0);
				}
				else if (tasks[it].type == cloud) {
					predecessor_ft_c.push_back(tasks[it].f_cloud);
				}
			}
			tasks[current].r_cloud = std::max(find_max(predecessor_ft_ws), find_max(predecessor_ft_c));
			tasks[current].r_cloud = std::max(availableTime[3], tasks[current].r_cloud);
			tasks[current].f_send = tasks[current].r_cloud + 3;
			tasks[current].f_cloud = tasks[current].f_send + 1;
			tasks[current].f_receive = tasks[current].f_cloud + 1;
			availableTime[3] = tasks[current].f_send;


		}


	}
}

auto calculate_final_finish_time(const vector<task> tasks, const vector<int> priority_list) {
	int last_task = priority_list[priority_list.size()-1];
	if (tasks[last_task].type == cloud) {
		return tasks[last_task].f_cloud;
	}
	else {
		return tasks[last_task].f_local;
	}
}

auto printSchedule(std::vector<task> vec) {
	for (auto it : vec) {
		if (it.type == local) {
			std::cout << "task no: " << it.id  << ",on core " << it.core	<< ", start time: " << it.r_local
				<< ", finish time: " << it.f_local <<endl;
		}
		else {
			std::cout << "task no: " << it.id << ",on cloud"<< ",send: " << it.r_cloud << ",finish send: "
				<< it.f_send << ",and finish receive :"<< it.f_receive << endl;
		}
	}
	return;
}

void printTimeLine(vector<task> tasks, int totalTime) {
	// 0=space 1=< 2=- 3=> 4=<--->
	//cout << totalTime << endl;
	totalTime += 3;
	vector<int> core0(totalTime,0), core1(totalTime, 0), core2(totalTime, 0), sendchannel(totalTime, 0), cloudexe(totalTime, 0), receivechannel(totalTime, 0);
	for (int i = 0; i < 10; i++) {
		if (tasks[i].type == local) {
			int start = tasks[i].r_local;
			int end = tasks[i].f_local;
			for (int j = start; j < end; j++) {
				if (tasks[i].core == 0) {
					if (j == start) core0[j] = 1;
					else if (j == end-1) core0[j] = 3;
					else core0[j] = 2;
				}
				else if (tasks[i].core == 1) {
					if (j == start) core1[j] = 1;
					else if (j == end - 1) core1[j] = 3;
					else core1[j] = 2;
				}
				else if (tasks[i].core == 2) {
					if (j == start) core2[j] = 1;
					else if (j == end - 1) core2[j] = 3;
					else core2[j] = 2;
				}
			}
		}
		else {
			int start = tasks[i].r_cloud;
			int sendEnd = tasks[i].f_receive;
			int exeEnd = tasks[i].f_cloud;
			int recEnd = tasks[i].f_send;
			//cout << "test: " << start << " " << recEnd << " " << exeEnd << " " << sendEnd << endl;
			for (int j = start; j < recEnd; j++) {
				if (j == start) receivechannel[j] = 1;
				else if (j == recEnd - 1) receivechannel[j] = 3;
				else receivechannel[j] = 2;
			}
			cloudexe[recEnd] = 4;
			
			sendchannel[exeEnd] = 4;
				
		}
	}
	cout << "TimeLine: ";
	for (int i = 0; i <= totalTime; i++) {
		if (i < 10) {
			cout << i << "    ";
		}
		else {
			cout << i << "   ";
		}
	}
	cout << endl;
	cout << "Core 0  : ";
	for (auto x : core0) {
		if (x == 0) cout << "     ";
		else if (x == 1) cout << "<----";
		else if (x == 2) cout << "------";
		else if (x == 3) cout << "---->";
	}
	cout << endl;
	cout << "Core 1  : ";
	for (auto x : core1) {
		if (x == 0) cout << "     ";
		else if (x == 1) cout << "<----";
		else if (x == 2) cout << "-----";
		else if (x == 3) cout << "---->";
	}
	cout << endl;
	cout << "Core 2  : ";
	for (auto x : core2) {
		if (x == 0) cout << "     ";
		else if (x == 1) cout << "<----";
		else if (x == 2) cout << "-----";
		else if (x == 3) cout << "---->";
	}
	cout << endl;
	cout << "Sending: ";
	for (auto x : receivechannel) {
		if (x == 0) cout << "     ";
		else if (x == 1) cout << "<----";
		else if (x == 2) cout << "-----";
		else if (x == 3) cout << "---->";
	}
	cout << endl;
	cout << "Cloud   : ";
	for (auto x : cloudexe) {
		if (x == 0) cout << "     ";
		else if (x == 1) cout << "<----";
		else if (x == 2) cout << "-----";
		else if (x == 4) cout << "<--->";
	}
	cout << endl;
	
	cout << "Reciving : ";
	for (auto x : sendchannel) {
		if (x == 0) cout << "     ";
		else if (x == 1) cout << "<----";
		else if (x == 2) cout << "-----";
		else if (x == 4) cout << "<--->";
	}
	cout << endl;
}

int energy(vector<task> tasks,vector<int> priority_list) {
	int total_energy = 0;
	for (int i = 0; i < priority_list.size(); i++) {
		const auto current = priority_list[i];
		if (tasks[current].type == local) {
			total_energy +=core_energy[tasks[current].core] *tasks[current].runtime[tasks[current].core];
		}
		else if (tasks[current].type == cloud) {
			total_energy += 1.5;
		}
	}
	return total_energy;
}

auto clear_task_schedule(std::vector<task>& tasks,
	const std::vector<int> priority_list) {
	for (int i = 0; i < priority_list.size(); i++) {
		int current_task = priority_list[i];
		tasks[current_task].core = 3;
		if (5 < find_min(tasks[current_task].runtime)) {
			tasks[current_task].type = cloud;
			tasks[current_task].f_local = 0;
			tasks[current_task].f_receive = 1000;
			tasks[current_task].f_send = 1000;
			tasks[current_task].f_cloud = 1000;
		}
		else {
			tasks[current_task].type = local;
			tasks[current_task].f_receive = 0;
			tasks[current_task].f_send = 0;
			tasks[current_task].f_cloud = 0;
			tasks[current_task].f_local = 1000;
		}
	}
}


void migration(vector<task>& tasks, vector<int> plist) {
	vector<int> availableTime(4, 0);
	for (int i = 0; i < plist.size(); i++) {
		auto current = plist[i];
		vector<int> newEnergy;
		vector<int> newFinishTime;
		vector<vector<int>> coreAvailable;
		vector<task> taskArrangement;

		auto first = plist.begin() + i + 1;
		auto last = plist.end();
		vector<int> following_tasks(first, last);

		for (int core = 0; core < 3; core++) {
			auto temp_tasks(tasks);
			auto temp_resource_available_times(availableTime);
			clear_task_schedule(temp_tasks, following_tasks);

			temp_tasks[current].type = local;
			temp_tasks[current].core = core;
			temp_tasks[current].f_local =temp_tasks[current].r_local +temp_tasks[current].runtime[core];
			temp_resource_available_times[core] =	temp_tasks[current].f_local;
			coreAvailable.push_back(temp_resource_available_times);
			taskArrangement.push_back(temp_tasks[current]);

			recalculate(temp_tasks, following_tasks,temp_resource_available_times);
			newEnergy.push_back(energy(temp_tasks, plist));
			// delta_finish_time.push_back(calculate_final_finish_time(tasks,
			// priority_list) - original_finish_time);

			//print_schedule(temp_tasks);
			//std::cout << "Energy = " << calculate_energy(temp_tasks, priority_list);
			//std::cout << "------------------------------------------" << std::endl;
		}
		auto temp_tasks(tasks);
		auto temp_resource_available_times(availableTime);
		clear_task_schedule(temp_tasks, following_tasks);

		temp_tasks[current].type = cloud;
		temp_tasks[current].r_cloud =max(temp_resource_available_times[3],temp_tasks[current].r_cloud);
		temp_tasks[current].f_send = temp_tasks[current].r_cloud + 3;
		temp_tasks[current].f_cloud =temp_tasks[current].f_send + 1;
		temp_tasks[current].f_receive =temp_tasks[current].f_cloud + 1;
		temp_resource_available_times[3] =temp_tasks[current].f_send;
		coreAvailable.push_back(temp_resource_available_times);
		taskArrangement.push_back(temp_tasks[current]);

		recalculate(temp_tasks, following_tasks,temp_resource_available_times);
		newEnergy.push_back(energy(temp_tasks, plist));
		// delta_finish_time.push_back(calculate_final_finish_time(tasks,
		// priority_list) - original_finish_time);

		//printSchedule(temp_tasks);
		//std::cout << "------------------ Energy = "
			//<< energy(temp_tasks, plist);
		//std::cout << " ------------------" << std::endl;

		auto best_place = find_argmin(newEnergy);
		tasks[current] = taskArrangement[best_place];
		availableTime = coreAvailable[best_place];
		//std::cout << "NEW" << std::endl;
	}
	return;
}

int main() {
	vector<task> tasks;
	string filename = "input2.txt";
	loadInput(filename, tasks);
	/*  //make sure input is correct
	for (int i = 0; i < 10; i++) {
		cout << tasks[i].id<<endl;
		for (int j = 0; j < tasks[i].predecessor.size(); j++) {
			cout << tasks[i].predecessor[j] << " ";
		}
		cout << endl;
		for (int j = 0; j < tasks[i].successor.size(); j++) {
			cout << tasks[i].successor[j] << " ";
		}
		cout << endl;
		cout << tasks[i].runtime[0] << " " << tasks[i].runtime[1] << " " << tasks[i].runtime[2] << endl<<endl;
	}
	*/
	prioritizing(tasks);
	//for (int i = 0; i < 10; i++) {
		//cout << tasks[i].priority << endl;
	//}

	vector<int> Plist = sort(tasks);
	 //make sure priority sort is correct
	//for (auto x : Plist) {
		//cout << x << endl;
	//}
	
	//cout << Plist[0] << endl;
	unitSelection(tasks, Plist);
	//cout << tasks[0].core << " " << tasks[0].f_local << endl;
	
	int totalTime = calculate_final_finish_time(tasks, Plist);
	//cout << totalTime << endl;
	cout << "Initial scheduling" << endl;
	printTimeLine(tasks, totalTime);

	int energyT = energy(tasks, Plist);
	cout << "Total energy consumed : " << energyT << endl;
	printSchedule(tasks);
	cout << endl << endl;
	migration(tasks, Plist);
	totalTime = calculate_final_finish_time(tasks, Plist);
	//cout << totalTime << endl;
	cout << "Optmized scheduling" << endl;
	printTimeLine(tasks, totalTime);

	energyT = energy(tasks, Plist);
	cout << "Total energy consumed : " << energyT << endl;
	printSchedule(tasks);

	return 0;
}
