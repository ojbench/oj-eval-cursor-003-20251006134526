#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
    
    Submission(string p, string s, int t) : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    int wrong_attempts = 0;           // Wrong attempts before first AC (or total if not solved and not frozen)
    int solve_time = -1;              // Time when solved (-1 if not solved)
    bool solved = false;
    int wrong_before_freeze = 0;      // Wrong attempts before freeze
    int submissions_after_freeze = 0; // Total submissions after freeze
    bool frozen = false;              // Is this problem frozen?
    bool was_unsolved_at_freeze = false; // Was unsolved when freeze happened
    vector<Submission> frozen_submissions; // Submissions made while frozen
    
    int getWrongAttempts() const {
        if (frozen) return wrong_before_freeze;
        return solved ? wrong_attempts : wrong_attempts;
    }
};

struct Team {
    string name;
    map<string, ProblemStatus> problems;
    vector<Submission> all_submissions;
    int ranking = 0;
    
    Team() {}
    Team(string n) : name(n) {}
    
    int getSolvedCount(bool include_frozen = false) const {
        int count = 0;
        for (const auto& p : problems) {
            if (p.second.solved && (!p.second.frozen || include_frozen)) {
                count++;
            }
        }
        return count;
    }
    
    int getPenaltyTime(bool include_frozen = false) const {
        int penalty = 0;
        for (const auto& p : problems) {
            if (p.second.solved && (!p.second.frozen || include_frozen)) {
                penalty += 20 * p.second.wrong_attempts + p.second.solve_time;
            }
        }
        return penalty;
    }
    
    vector<int> getSolveTimes(bool include_frozen = false) const {
        vector<int> times;
        for (const auto& p : problems) {
            if (p.second.solved && (!p.second.frozen || include_frozen)) {
                times.push_back(p.second.solve_time);
            }
        }
        sort(times.rbegin(), times.rend());
        return times;
    }
    
    string getProblemDisplay(const string& prob_name) const {
        auto it = problems.find(prob_name);
        if (it == problems.end() || (!it->second.solved && it->second.wrong_attempts == 0 && !it->second.frozen)) {
            return ".";
        }
        
        const ProblemStatus& ps = it->second;
        
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

class ICPCSystem {
private:
    map<string, Team> teams;
    vector<string> team_order;
    bool competition_started = false;
    int duration_time = 0;
    int problem_count = 0;
    vector<string> problem_names;
    bool is_frozen = false;
    
    bool compareTeams(const Team& a, const Team& b) const {
        int solved_a = a.getSolvedCount();
        int solved_b = b.getSolvedCount();
        
        if (solved_a != solved_b) return solved_a > solved_b;
        
        int penalty_a = a.getPenaltyTime();
        int penalty_b = b.getPenaltyTime();
        
        if (penalty_a != penalty_b) return penalty_a < penalty_b;
        
        vector<int> times_a = a.getSolveTimes();
        vector<int> times_b = b.getSolveTimes();
        
        for (size_t i = 0; i < min(times_a.size(), times_b.size()); i++) {
            if (times_a[i] != times_b[i]) return times_a[i] < times_b[i];
        }
        
        return a.name < b.name;
    }
    
    void updateRankings() {
        vector<Team*> team_list;
        for (auto& p : teams) {
            team_list.push_back(&p.second);
        }
        
        sort(team_list.begin(), team_list.end(), 
             [this](const Team* a, const Team* b) { return compareTeams(*a, *b); });
        
        for (size_t i = 0; i < team_list.size(); i++) {
            team_list[i]->ranking = i + 1;
        }
    }
    
    void printScoreboard() const {
        vector<const Team*> team_list;
        for (const auto& p : teams) {
            team_list.push_back(&p.second);
        }
        
        sort(team_list.begin(), team_list.end(), 
             [this](const Team* a, const Team* b) { return compareTeams(*a, *b); });
        
        for (const auto* team : team_list) {
            cout << team->name << " " << team->ranking << " " 
                 << team->getSolvedCount() << " " << team->getPenaltyTime();
            
            for (const string& prob : problem_names) {
                cout << " " << team->getProblemDisplay(prob);
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
            team_order.push_back(team_name);
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
            
            for (int i = 0; i < prob_count; i++) {
                problem_names.push_back(string(1, 'A' + i));
            }
            
            updateRankings();
            cout << "[Info]Competition starts.\n";
        }
    }
    
    void submit(const string& problem, const string& team_name, 
                const string& status, int time) {
        Team& team = teams[team_name];
        team.all_submissions.push_back(Submission(problem, status, time));
        
        ProblemStatus& ps = team.problems[problem];
        
        // Check if this submission happens after freeze and problem was unsolved at freeze
        if (is_frozen && ps.was_unsolved_at_freeze) {
            if (!ps.frozen) {
                ps.frozen = true;
                ps.wrong_before_freeze = ps.wrong_attempts;
            }
            ps.submissions_after_freeze++;
            ps.frozen_submissions.push_back(Submission(problem, status, time));
        } else if (!ps.solved) {
            // Normal submission processing (before freeze or problem was already solved at freeze)
            if (status == "Accepted") {
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
            
            // Mark all currently unsolved problems as potentially frozen
            for (auto& team_pair : teams) {
                Team& team = team_pair.second;
                for (const string& prob : problem_names) {
                    ProblemStatus& ps = team.problems[prob];
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
        printScoreboard();
        
        // Perform scroll operation
        while (true) {
            Team* lowest_team = nullptr;
            int lowest_rank = -1;
            
            // Find lowest-ranked team with frozen problems
            for (auto& p : teams) {
                Team& team = p.second;
                bool has_frozen_prob = false;
                
                for (auto& prob : team.problems) {
                    if (prob.second.frozen) {
                        has_frozen_prob = true;
                        break;
                    }
                }
                
                if (has_frozen_prob && (lowest_team == nullptr || team.ranking > lowest_rank)) {
                    lowest_team = &team;
                    lowest_rank = team.ranking;
                }
            }
            
            if (lowest_team == nullptr) break;
            
            // Find smallest problem number that is frozen
            string smallest_frozen;
            for (const string& prob : problem_names) {
                auto it = lowest_team->problems.find(prob);
                if (it != lowest_team->problems.end() && it->second.frozen) {
                    smallest_frozen = prob;
                    break;
                }
            }
            
            // Save old ranking for this team
            string team_name = lowest_team->name;
            int old_rank = lowest_team->ranking;
            
            // Unfreeze the problem and process frozen submissions
            ProblemStatus& ps = lowest_team->problems[smallest_frozen];
            ps.frozen = false;
            
            for (const Submission& sub : ps.frozen_submissions) {
                if (!ps.solved) {
                    if (sub.status == "Accepted") {
                        ps.solved = true;
                        ps.solve_time = sub.time;
                    } else {
                        ps.wrong_attempts++;
                    }
                }
            }
            ps.frozen_submissions.clear();
            
            // Update rankings
            updateRankings();
            
            // Check if ranking changed
            int new_rank = lowest_team->ranking;
            if (new_rank < old_rank) {
                // Find team at the new rank position (excluding the moving team)
                string replaced_team;
                for (const auto& p : teams) {
                    if (p.second.name != team_name && p.second.ranking == new_rank + 1) {
                        replaced_team = p.second.name;
                        break;
                    }
                }
                
                if (!replaced_team.empty()) {
                    cout << team_name << " " << replaced_team << " " 
                         << lowest_team->getSolvedCount() << " " 
                         << lowest_team->getPenaltyTime() << "\n";
                }
            }
        }
        
        printScoreboard();
        
        // Unfreeze state and reset flags
        is_frozen = false;
        for (auto& team_pair : teams) {
            Team& team = team_pair.second;
            for (auto& prob : team.problems) {
                prob.second.was_unsolved_at_freeze = false;
                prob.second.frozen = false;
                prob.second.submissions_after_freeze = 0;
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
        
        for (int i = team.all_submissions.size() - 1; i >= 0; i--) {
            const Submission& sub = team.all_submissions[i];
            
            bool prob_match = (problem_filter == "ALL" || sub.problem == problem_filter);
            bool status_match = (status_filter == "ALL" || sub.status == status_filter);
            
            if (prob_match && status_match) {
                cout << team_name << " " << sub.problem << " " 
                     << sub.status << " " << sub.time << "\n";
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
