#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    char problem;
    char status;
    int time;
    
    Submission(char p, char s, int t) : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    int wrong_attempts = 0;
    int solve_time = -1;
    bool solved = false;
    int wrong_before_freeze = 0;
    int submissions_after_freeze = 0;
    bool frozen = false;
    bool was_unsolved_at_freeze = false;
    vector<Submission> frozen_submissions;
};

struct Team {
    string name;
    ProblemStatus problems[26];
    vector<Submission> all_submissions;
    int ranking = 0;
    int cached_solved = 0;
    int cached_penalty = 0;
    vector<int> cached_times;
    
    Team() {}
    Team(string n) : name(n) {}
    
    void updateCache(int problem_count) {
        cached_solved = 0;
        cached_penalty = 0;
        cached_times.clear();
        
        for (int i = 0; i < problem_count; i++) {
            if (problems[i].solved && !problems[i].frozen) {
                cached_solved++;
                cached_penalty += 20 * problems[i].wrong_attempts + problems[i].solve_time;
                cached_times.push_back(problems[i].solve_time);
            }
        }
        sort(cached_times.rbegin(), cached_times.rend());
    }
    
    string getProblemDisplay(int prob_idx) const {
        const ProblemStatus& ps = problems[prob_idx];
        
        if (!ps.solved && ps.wrong_attempts == 0 && !ps.frozen) {
            return ".";
        }
        
        if (ps.frozen) {
            string result;
            if (ps.wrong_before_freeze > 0) {
                result = "-" + to_string(ps.wrong_before_freeze);
            } else {
                result = "0";
            }
            result += "/" + to_string(ps.submissions_after_freeze);
            return result;
        } else if (ps.solved) {
            return ps.wrong_attempts > 0 ? ("+" + to_string(ps.wrong_attempts)) : "+";
        } else {
            return "-" + to_string(ps.wrong_attempts);
        }
    }
};

bool compareTeams(const Team* a, const Team* b) {
    if (a->cached_solved != b->cached_solved) 
        return a->cached_solved > b->cached_solved;
    
    if (a->cached_penalty != b->cached_penalty) 
        return a->cached_penalty < b->cached_penalty;
    
    size_t minsize = min(a->cached_times.size(), b->cached_times.size());
    for (size_t i = 0; i < minsize; i++) {
        if (a->cached_times[i] != b->cached_times[i]) 
            return a->cached_times[i] < b->cached_times[i];
    }
    
    return a->name < b->name;
}

char statusToChar(const string& status) {
    if (status == "Accepted") return 0;
    if (status == "Wrong_Answer") return 1;
    if (status == "Runtime_Error") return 2;
    return 3;
}

string charToStatus(char c) {
    if (c == 0) return "Accepted";
    if (c == 1) return "Wrong_Answer";
    if (c == 2) return "Runtime_Error";
    return "Time_Limit_Exceed";
}

class ICPCSystem {
private:
    map<string, Team> teams;
    bool competition_started = false;
    int duration_time = 0;
    int problem_count = 0;
    bool is_frozen = false;
    
    void updateRankings() {
        vector<Team*> team_list;
        team_list.reserve(teams.size());
        
        for (auto& p : teams) {
            p.second.updateCache(problem_count);
            team_list.push_back(&p.second);
        }
        
        sort(team_list.begin(), team_list.end(), compareTeams);
        
        for (size_t i = 0; i < team_list.size(); i++) {
            team_list[i]->ranking = i + 1;
        }
    }
    
    void printScoreboard(vector<Team*>* presorted = nullptr) {
        vector<Team*> team_list;
        
        if (presorted) {
            team_list = *presorted;
        } else {
            team_list.reserve(teams.size());
            for (auto& p : teams) {
                team_list.push_back(&p.second);
            }
            sort(team_list.begin(), team_list.end(), compareTeams);
        }
        
        for (auto* team : team_list) {
            cout << team->name << " " << team->ranking << " " 
                 << team->cached_solved << " " << team->cached_penalty;
            
            for (int i = 0; i < problem_count; i++) {
                cout << " " << team->getProblemDisplay(i);
            }
            cout << "\n";
        }
    }
    
public:
    void addTeam(const string& team_name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started.\n";
        } else if (teams.find(team_name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
        } else {
            teams[team_name] = Team(team_name);
            cout << "[Info]Add successfully.\n";
        }
    }
    
    void startCompetition(int duration, int prob_count) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started.\n";
        } else {
            competition_started = true;
            duration_time = duration;
            problem_count = prob_count;
            
            updateRankings();
            cout << "[Info]Competition starts.\n";
        }
    }
    
    void submit(const string& problem, const string& team_name, 
                const string& status, int time) {
        Team& team = teams[team_name];
        int prob_idx = problem[0] - 'A';
        char status_char = statusToChar(status);
        
        team.all_submissions.push_back(Submission(problem[0], status_char, time));
        
        ProblemStatus& ps = team.problems[prob_idx];
        
        if (is_frozen && ps.was_unsolved_at_freeze) {
            if (!ps.frozen) {
                ps.frozen = true;
                ps.wrong_before_freeze = ps.wrong_attempts;
            }
            ps.submissions_after_freeze++;
            ps.frozen_submissions.push_back(Submission(problem[0], status_char, time));
        } else if (!ps.solved) {
            if (status_char == 0) {
                ps.solved = true;
                ps.solve_time = time;
            } else {
                ps.wrong_attempts++;
            }
        }
    }
    
    void flush() {
        updateRankings();
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze() {
        if (is_frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
        } else {
            is_frozen = true;
            
            for (auto& team_pair : teams) {
                Team& team = team_pair.second;
                for (int i = 0; i < problem_count; i++) {
                    ProblemStatus& ps = team.problems[i];
                    if (!ps.solved) {
                        ps.was_unsolved_at_freeze = true;
                    }
                }
            }
            
            cout << "[Info]Freeze scoreboard.\n";
        }
    }
    
    void scroll() {
        if (!is_frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        updateRankings();
        
        vector<Team*> sorted_teams;
        sorted_teams.reserve(teams.size());
        for (auto& p : teams) {
            sorted_teams.push_back(&p.second);
        }
        sort(sorted_teams.begin(), sorted_teams.end(), compareTeams);
        
        printScoreboard(&sorted_teams);
        
        while (true) {
            Team* lowest_team = nullptr;
            int smallest_frozen_idx = -1;
            
            for (int i = sorted_teams.size() - 1; i >= 0; i--) {
                Team* team = sorted_teams[i];
                
                for (int j = 0; j < problem_count; j++) {
                    if (team->problems[j].frozen) {
                        smallest_frozen_idx = j;
                        lowest_team = team;
                        break;
                    }
                }
                
                if (lowest_team) break;
            }
            
            if (lowest_team == nullptr) break;
            
            string team_name = lowest_team->name;
            int old_rank = lowest_team->ranking;
            
            ProblemStatus& ps = lowest_team->problems[smallest_frozen_idx];
            ps.frozen = false;
            
            for (const Submission& sub : ps.frozen_submissions) {
                if (!ps.solved) {
                    if (sub.status == 0) {
                        ps.solved = true;
                        ps.solve_time = sub.time;
                    } else {
                        ps.wrong_attempts++;
                    }
                }
            }
            ps.frozen_submissions.clear();
            
            lowest_team->updateCache(problem_count);
            
            // Remove team from current position
            int current_pos = old_rank - 1;
            sorted_teams.erase(sorted_teams.begin() + current_pos);
            
            // Binary search for new position
            int new_pos = lower_bound(sorted_teams.begin(), sorted_teams.end(), lowest_team, compareTeams) - sorted_teams.begin();
            sorted_teams.insert(sorted_teams.begin() + new_pos, lowest_team);
            
            // Update rankings only for affected range
            int start = min(current_pos, new_pos);
            int end = max(current_pos, new_pos) + 1;
            for (int i = start; i <= end && i < (int)sorted_teams.size(); i++) {
                sorted_teams[i]->ranking = i + 1;
            }
            
            int new_rank = new_pos + 1;
            if (new_rank < old_rank) {
                if (new_rank < (int)sorted_teams.size()) {
                    string replaced_team = sorted_teams[new_rank]->name;
                    if (replaced_team != team_name) {
                        cout << team_name << " " << replaced_team << " " 
                             << lowest_team->cached_solved << " " << lowest_team->cached_penalty << "\n";
                    }
                }
            }
        }
        
        printScoreboard(&sorted_teams);
        
        is_frozen = false;
        for (auto& team_pair : teams) {
            Team& team = team_pair.second;
            for (int i = 0; i < problem_count; i++) {
                team.problems[i].was_unsolved_at_freeze = false;
                team.problems[i].frozen = false;
                team.problems[i].submissions_after_freeze = 0;
            }
        }
    }
    
    void queryRanking(const string& team_name) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        
        if (is_frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        
        cout << team_name << " NOW AT RANKING " << teams[team_name].ranking << "\n";
    }
    
    void querySubmission(const string& team_name, const string& problem_filter, 
                        const string& status_filter) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        const Team& team = teams[team_name];
        char prob_char = (problem_filter == "ALL") ? 127 : problem_filter[0];
        char status_char = (status_filter == "ALL") ? 127 : statusToChar(status_filter);
        
        for (int i = team.all_submissions.size() - 1; i >= 0; i--) {
            const Submission& sub = team.all_submissions[i];
            
            bool prob_match = (prob_char == 127 || sub.problem == prob_char);
            bool status_match = (status_char == 127 || sub.status == status_char);
            
            if (prob_match && status_match) {
                cout << team_name << " " << sub.problem << " " 
                     << charToStatus(sub.status) << " " << sub.time << "\n";
                return;
            }
        }
        
        cout << "Cannot find any submission.\n";
    }
    
    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);
    
    ICPCSystem system;
    string line;
    
    while (getline(cin, line)) {
        if (line.empty()) continue;
        
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string team_name;
            iss >> team_name;
            system.addTeam(team_name);
        } else if (cmd == "START") {
            string duration_str, problem_str;
            int duration, prob_count;
            iss >> duration_str >> duration >> problem_str >> prob_count;
            system.startCompetition(duration, prob_count);
        } else if (cmd == "SUBMIT") {
            string problem, by, team_name, with, status, at;
            int time;
            iss >> problem >> by >> team_name >> with >> status >> at >> time;
            system.submit(problem, team_name, status, time);
        } else if (cmd == "FLUSH") {
            system.flush();
        } else if (cmd == "FREEZE") {
            system.freeze();
        } else if (cmd == "SCROLL") {
            system.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.queryRanking(team_name);
        } else if (cmd == "QUERY_SUBMISSION") {
            string team_name, where, filter1, and_str, filter2;
            iss >> team_name >> where >> filter1 >> and_str >> filter2;
            
            string problem = filter1.substr(8);
            string status = filter2.substr(7);
            
            system.querySubmission(team_name, problem, status);
        } else if (cmd == "END") {
            system.end();
            break;
        }
    }
    
    return 0;
}
