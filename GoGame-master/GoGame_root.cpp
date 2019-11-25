#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <omp.h>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

using namespace std;

void timing(double *wcTime, double *cpuTime) {
    struct timeval tp;
    struct rusage ruse;

    gettimeofday(&tp, NULL);
    *wcTime = (double)(tp.tv_sec + tp.tv_usec / 1000000.0);

    getrusage(RUSAGE_SELF, &ruse);
    *cpuTime = (double)(ruse.ru_utime.tv_sec + ruse.ru_utime.tv_usec / 1000000.0);
}

class Position {
public:
    vector<vector<int>> board;
    vector<vector<bool>> is_visit;
    int player;
    int ko_row, ko_col, ko_turn;

    int win_player;
    bool player1_pass;
    bool player2_pass;

    Position() {
        player = 1;
        player1_pass = false;
        player2_pass = false;
        ko_row = -1;
        ko_col = -1;
        ko_turn = 0;
        win_player = 0;
        for (int i = 0; i < 9; ++i) {
            vector<int> tmp;
            for (int i = 0; i < 9; ++i)
                tmp.push_back(0);
            board.push_back(tmp);
        }
        for (int i = 0; i < 9; ++i) {
            vector<bool> tmp;
            for (int i = 0; i < 9; ++i)
                tmp.push_back(false);
            is_visit.push_back(tmp);
        }
    }

    Position(const Position &t) {
        board = t.board;
        player1_pass = t.player1_pass;
        player2_pass = t.player2_pass;
        ko_row = t.ko_row;
        ko_col = t.ko_col;
        ko_turn = t.ko_turn;
        win_player = t.win_player;
        player = t.player;
        for (int i = 0; i < 9; ++i) {
            vector<bool> tmp;
            for (int i = 0; i < 9; ++i)
                tmp.push_back(false);
            is_visit.push_back(tmp);
        }
    }

    Position(vector<vector<int>> board_, int p) {
        player = p;
        board = board_;
        for (int i = 0; i < 9; ++i) {
            vector<bool> tmp;
            for (int i = 0; i < 9; ++i)
                tmp.push_back(false);
            is_visit.push_back(tmp);
        }
    }

    bool is_valid(int row, int col) {
        if (board[row][col] != 0 || (row == ko_row && col == ko_col))
            return false;
        else {
            Position tmp(board, player);
            board[row][col] = player;
            if (count_eyes(row + 1, col, -player) == 0) {
                remove_stone(-player);
            }
            if (count_eyes(row - 1, col, -player) == 0) {
                remove_stone(-player);
            }
            if (count_eyes(row, col + 1, -player) == 0) {
                remove_stone(-player);
            }
            if (count_eyes(row, col - 1, -player) == 0) {
                remove_stone(-player);
            }

            int stone_num = 0;

            int t = count_eyes_zero(row, col, player, stone_num);
            if (t == 0 || (t == 1 && stone_num > 5)) {
                board = tmp.board;
                return false;
            }
            board = tmp.board;
            player = tmp.player;
            return true;
        }
    }

    int make_move(int row, int col) {
        if (row == -2 && col == -2) { // pass for human player
            player = -player;
            ko_row = -1;
            ko_col = -1;
            return -2;
        }
        if (board[row][col] != 0 || (row == ko_row && col == ko_col))
            return -1;
        else {
            vector<vector<int>> tmp_board = board;
            int ko_turn_ = ko_turn;
            int ko_row_ = ko_row;
            int ko_col_ = ko_col;

            board[row][col] = player;
            ko_row = -1;
            ko_col = -1;
            if (count_eyes(row + 1, col, -player) == 0) {
                if (remove_stone(-player) == 1) {
                    ko_row = row + 1;
                    ko_col = col;
                    ko_turn++;
                }
            }
            if (count_eyes(row - 1, col, -player) == 0) {
                if (remove_stone(-player) == 1) {
                    ko_row = row - 1;
                    ko_col = col;
                    ko_turn++;
                }
            }
            if (count_eyes(row, col + 1, -player) == 0) {
                if (remove_stone(-player) == 1) {
                    ko_row = row;
                    ko_col = col + 1;
                    ko_turn++;
                }
            }
            if (count_eyes(row, col - 1, -player) == 0) {
                if (remove_stone(-player) == 1) {
                    ko_row = row;
                    ko_col = col - 1;
                    ko_turn++;
                }
            }

            int stone_num = 0;

            int t = count_eyes_zero(row, col, player, stone_num);
            if (t == 0 || (t == 1 && stone_num > 5)) {
                board = tmp_board;
                ko_row = ko_row_;
                ko_col = ko_col_;
                ko_turn_ = ko_turn;
                return -1;
            }

            if (ko_row == -1)
                ko_turn = 0;
        }
        if (player == 1)
            player1_pass = false;
        else
            player2_pass = false;
        player = -player;
        return 0;
    }

    bool is_pass() {
        if (ko_turn > 9) {
            return true;
        }
        for (int i = 0; i < 9; ++i)
            for (int j = 0; j < 9; ++j) {
                if (is_valid(i, j)) {

                    return false;
                }
            }

        return true;
    }

    void pass_move() {
        if (ko_turn > 9) {
            win_player = 0;
        }
        if (ko_row == -1 && ko_col == -1) {
            if (player == 1)
                player1_pass = true;
            else
                player2_pass = true;
        } else {
            ko_row = -1;
            ko_col = -1;
        }

        player = -player;
    }

    bool game_over() {
        if (ko_turn > 9) {
            return true;
        }
        if (player1_pass && player2_pass) {
            double player1_res = 0.0, player2_res = 0.0;
            for (int i = 0; i < 9; ++i) {
                for (int j = 0; j < 9; ++j) {
                    if (board[i][j] == 1)
                        player1_res++;
                    if (board[i][j] == -1)
                        player2_res++;
                    if (board[i][j] == 0) {
                        bool player1_stone = false, player2_stone = false, blank = false;
                        if (i - 1 >= 0) {
                            if (board[i - 1][j] == 1)
                                player1_stone = true;
                            if (board[i - 1][j] == -1)
                                player2_stone = true;
                            if (board[i - 1][j] == 0)
                                blank = true;
                        }
                        if (i + 1 <= 8) {
                            if (board[i + 1][j] == 1)
                                player1_stone = true;
                            if (board[i + 1][j] == -1)
                                player2_stone = true;
                            if (board[i + 1][j] == 0)
                                blank = true;
                        }
                        if (j - 1 >= 0) {
                            if (board[i][j - 1] == 1)
                                player1_stone = true;
                            if (board[i][j - 1] == -1)
                                player2_stone = true;
                            if (board[i][j - 1] == 0)
                                blank = true;
                        }
                        if (j + 1 <= 8) {
                            if (board[i][j + 1] == 1)
                                player1_stone = true;
                            if (board[i][j + 1] == -1)
                                player2_stone = true;
                            if (board[i][j + 1] == 0)
                                blank = true;
                        }
                        int true_value = 0;
                        if (player1_stone == true)
                            true_value++;
                        if (player2_stone == true)
                            true_value++;
                        if (blank == true)
                            true_value++;
                        if (true_value >= 2) {
                            player1_res += 0.5;
                            player2_res += 0.5;
                        } else {
                            if (player1_stone == true)
                                player1_res += 1;
                            if (player2_stone == true)
                                player2_res += 1;
                        }
                    }
                }
            }
            if (player1_res > player2_res + 7.5)
                win_player = 1;
            else
                win_player = -1;
            //cout << "player1: " << player1_res << " player2: "<< player2_res << " " << win_player << endl;
        }
        return player1_pass && player2_pass;
    }

    int who_win() {
        //cout << win_player << endl;
        return win_player;
    }

    void print() const {
        cout << "-------------------------" << endl;
        cout << "  0 1 2 3 4 5 6 7 8" << endl;
        for (int i = 0; i < 9; ++i) {
            cout << " " << i;
            for (int j = 0; j < 9; ++j) {
                if (board[i][j] == 1)
                    cout << "*"
                         << " ";
                if (board[i][j] == 0)
                    cout << "."
                         << " ";
                if (board[i][j] == -1)
                    cout << "@"
                         << " ";
            }
            cout << endl;
        }
    }

    const string to_string() const {
        string res;
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                if (board[i][j] == 1)
                    res.push_back('*');
                if (board[i][j] == 0)
                    res.push_back('.');
                if (board[i][j] == -1)
                    res.push_back('@');
            }
        }
        if (player == 1)
            res.push_back('#');
        else
            res.push_back('&');
        return res;
    }

    bool operator==(const Position &p) const {
        return (board == p.board) && (player == p.player);
    }

private:
    int count_eyes(int row, int col, int player_) {
        if (row < 0 || row > 8 || col < 0 || col > 8 || board[row][col] != player_)
            return -1;
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                is_visit[i][j] = false;
            }
        }
        int res = 0;
        dfs(row, col, board[row][col], res);
        //cout << "----" << res << "----"<< endl;
        return res;
    }

    void dfs(int row, int col, int player_, int &res) {
        if (row < 0 || row > 8 || col < 0 || col > 8 || is_visit[row][col] == true || board[row][col] == -player_)
            return;
        is_visit[row][col] = true;
        if (board[row][col] == player_) {
            dfs(row + 1, col, player_, res);
            dfs(row - 1, col, player_, res);
            dfs(row, col + 1, player_, res);
            dfs(row, col - 1, player_, res);
        }
        if (board[row][col] == 0) {
            res += 1;
        }
    }

    int count_eyes_zero(int row, int col, int player_, int &stone_num) {
        if (row < 0 || row > 8 || col < 0 || col > 8 || board[row][col] != player_)
            return 10;
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                is_visit[i][j] = false;
            }
        }
        int res = 0;
        stone_num = dfs_fast_end(row, col, board[row][col], res);
        //cout << "----" << res << "----"<< endl;
        return res;
    }

    int dfs_fast_end(int row, int col, int player_, int &res) {
        if (res > 1 || row < 0 || row > 8 || col < 0 || col > 8 || is_visit[row][col] == true || board[row][col] == -player_)
            return 0;
        is_visit[row][col] = true;
        if (board[row][col] == player_) {
            return dfs_fast_end(row + 1, col, player_, res) +
                   dfs_fast_end(row - 1, col, player_, res) +
                   dfs_fast_end(row, col + 1, player_, res) +
                   dfs_fast_end(row, col - 1, player_, res) + 1;
        }
        if (board[row][col] == 0) {
            res += 1;
        }
        return 0;
    }

    int remove_stone(int player_) {
        int res = 0;
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                if (is_visit[i][j] == true && board[i][j] == player_) {
                    board[i][j] = 0;
                    res++;
                }
            }
        }
        return res;
    }
};

class value {
public:
    value *last_pos_value;
    double total_game;
    double total_win;

    value(value *p_v, double g, double w) {
        last_pos_value = p_v;
        total_win = w;
        total_game = g;
    }
    void print() {
        cout << total_game << " " << total_win << endl;
    }
};

void get_next_pos(Position *s, vector<Position> &next_pos) {
    next_pos.clear();

    if (s->is_pass()) {
        Position tmp(*s);
        tmp.pass_move();
        next_pos.push_back(tmp);
        return;
    }

    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            Position tmp(*s);

            if (tmp.make_move(i, j) != -1)
                next_pos.push_back(tmp);
        }
    }
}

namespace std {
template <>
struct hash<Position> {
    size_t
    operator()(const Position obj) const {
        return hash<string>()(obj.to_string());
    }
};
} // namespace std

unordered_map<Position, value *> tree;
void random_play(Position *s) {
    if (!s->is_pass()) {
        int x, y;
        do {
            x = rand() % 9;
            y = rand() % 9;
        } while (s->make_move(x, y) == -1);
    } else {
        s->pass_move();
    }
}

void manual_play(Position *s) {
    if (!s->is_pass()) {
        int x, y;
        do {
            cin >> x >> y;
        } while (s->make_move(x, y) == -1);
    } else {
        s->pass_move();
    }
}

Position *mcts_play(Position *s, int iters, int playout_num, int thread_num) {

    int my_player = s->player;
    if (s->is_pass()) {
        s->pass_move();
        return s;
    }
    vector<Position> next_poss[thread_num];
    unordered_map<Position, value *> localTrees[thread_num];
#pragma omp parallel
    {

        //if(s->game_over()) impossible
        bool all_in;
        Position *t;

        int threadIndex = omp_get_thread_num();
        localTrees[threadIndex].clear();
        localTrees[threadIndex][*s] = new value(NULL, 0, 0);

        value *root_v = localTrees[threadIndex][*s];

        vector<Position> next_pos = next_poss[threadIndex];
        Position *s_ = new Position(*s);

        for (int i = 0; i < iters / thread_num; ++i) {
            t = s_;

            while (!t->game_over()) {
                all_in = true;

                get_next_pos(t, next_pos);

                // #pragma omp critical
                // 	{
                // 		cout << "~~~~~~~~~~~~~~~~~" << endl;

                // 		for(int ss = 0; ss < next_pos.size(); ++ss)next_pos[ss].print();
                // 		cout << "~~~~~~~~~~~~~~~~~" << endl;
                // 		fflush(stdout);
                // 	}

                int index = 0;
                for (int j = 0; j < next_pos.size(); ++j) {

                    if (tree.find(next_pos[j]) == tree.end() && localTrees[threadIndex].find(next_pos[j]) == localTrees[threadIndex].end()) {
                        all_in = false;
                        index = j;
                        break;
                    }
                    // else if (tree.find(next_pos[j]) != tree.end() && localTrees[threadIndex].find(next_pos[j]) == localTrees[threadIndex].end()){
                    // 	//add 0 0 to local tree
                    // 	localTrees[threadIndex][next_pos[j]] = new value(localTrees[threadIndex][*t], 0.0, 0.0);
                    // }
                }

                //if all_in then tree policy else expand and break;
                if (all_in == false) {

                    localTrees[threadIndex][next_pos[index]] = new value(localTrees[threadIndex][*t], 0.0, 0.0);
                    t = new Position(next_pos[index]);
                    break;
                } else {
                    //find the best current children, UCTS strategy

                    double z = 0.2;
                    value *v = localTrees[threadIndex][*t];
                    ;
                    double T = v->total_game;
                    if (tree.find(*t) != tree.end())
                        T += tree[*t]->total_game;
                    Position *tmp_pos = NULL;
                    int best_row, best_col;
                    if (t->player == my_player) {
                        double ucb = -10000000000000.0;
                        for (int j = 0; j < next_pos.size(); ++j) {
                            value *vv1 = NULL, *vv2 = NULL;
                            if (tree.find(next_pos[j]) != tree.end())
                                vv1 = tree[next_pos[j]];
                            if (localTrees[threadIndex].find(next_pos[j]) != localTrees[threadIndex].end())
                                vv2 = localTrees[threadIndex][next_pos[j]];
                            double pj = 0.0, nj = 0.0;
                            if (vv1 != NULL) {
                                pj += vv1->total_win;
                                nj += vv1->total_game;
                            }

                            if (vv2 != NULL) {
                                pj += vv2->total_win;
                                nj += vv2->total_game;
                            }

                            double tmp_ucb = pj / nj + sqrt(z * log(T) / nj);
                            if (ucb < tmp_ucb) {
                                ucb = tmp_ucb;
                                tmp_pos = &next_pos[j];
                            }
                        }
                    } else {
                        double ucb = 10000000000000.0;
                        for (int j = 0; j < next_pos.size(); ++j) {
                            value *vv1 = NULL, *vv2 = NULL;
                            if (tree.find(next_pos[j]) != tree.end())
                                vv1 = tree[next_pos[j]];
                            if (localTrees[threadIndex].find(next_pos[j]) != localTrees[threadIndex].end())
                                vv2 = localTrees[threadIndex][next_pos[j]];
                            double pj = 0.0, nj = 0.0;
                            if (vv1 != NULL) {
                                pj += vv1->total_win;
                                nj += vv1->total_game;
                            }

                            if (vv2 != NULL) {
                                pj += vv2->total_win;
                                nj += vv2->total_game;
                            }

                            double tmp_ucb = pj / nj - sqrt(z * log(T) / nj);
                            if (ucb > tmp_ucb) {
                                ucb = tmp_ucb;
                                tmp_pos = &next_pos[j];
                            }
                        }
                    }
                    if (localTrees[threadIndex].find(*tmp_pos) != localTrees[threadIndex].end())
                        localTrees[threadIndex][*tmp_pos]->last_pos_value = v;
                    else
                        localTrees[threadIndex][*tmp_pos] = new value(v, 0.0, 0.0);
                    if (t != s_)
                        delete t;
                    t = new Position(*tmp_pos);
                }
            }

            //playout policy
            double total_g = 0, total_w = 0;
            if (t->game_over()) {
                if (t->who_win() == my_player) {
                    total_g = playout_num;
                    total_w = playout_num;
                } else if (t->who_win() == 0) {
                    total_g = playout_num;
                    total_w = playout_num / 2.0;
                } else {
                    total_g = playout_num;
                    total_w = 0;
                }
            } else {
                int j;
                for (j = 0; j < playout_num; ++j) {

                    Position *tt = new Position(*t);
                    while (!tt->game_over()) {
                        if (!tt->is_pass()) {
                            int x, y;
                            do {
                                x = rand() % 9;
                                y = rand() % 9;
                            } while (tt->make_move(x, y) == -1);
                        } else {
                            tt->pass_move();
                        }
                    }

                    if (tt->who_win() == my_player) {
                        total_g += 1;
                        total_w += 1;
                    } else if (tt->who_win() == 0) {
                        total_g += 1;
                        total_w += 0.5;
                    } else {
                        total_g += 1;
                        total_w += 0;
                    }
                    delete tt;
                }
            }
            //back propagate result
            // #pragma omp critical
            // {
            // 	cout << "------------------" << endl;
            // 	cout << threadIndex << endl;
            // 	t->print();
            // 	localTrees[threadIndex][*t] -> print();
            // 	cout << "------------------" << endl;
            // 	fflush(stdout);
            // }

            value *v_ = localTrees[threadIndex][*t];
            v_->total_game += total_g;
            v_->total_win += total_w;
            int loop_num = 0;
            while (v_ != root_v && loop_num++ < 500) {
                v_ = v_->last_pos_value;
                v_->total_game += total_g;
                v_->total_win += total_w;
            }
        }
    }
    //merge local trees to global trees
    for (int tt = 0; tt < thread_num; ++tt) {
        for (auto p = localTrees[tt].begin(); p != localTrees[tt].end(); p++) {
            // cout << "**************" << tt << "*************** " <<endl;
            // p->first.print();
            // cout << p->second->total_game << " " << p->second->total_win << endl;
            // cout << "******************************************" << endl;
            if (tree.find(p->first) == tree.end()) {
                if (p->second->total_game != 0)
                    tree[p->first] = new value(NULL, p->second->total_game, p->second->total_win);
            } else {
                tree[p->first]->total_game += p->second->total_game;
                tree[p->first]->total_win += p->second->total_win;
            }
            delete p->second;
        }
    }
    //choose the best move;
    vector<Position> next_pos = next_poss[0];
    get_next_pos(s, next_pos);
    double average = -10.0;
    int index = -1;
    //cout << "--------" << endl;
    for (int i = 0; i < next_pos.size(); ++i) {
        if (tree.find(next_pos[i]) != tree.end()) {
            value *v = tree[next_pos[i]];
            //cout << v->total_game <<" " << v->total_win << endl;
            if (average < v->total_win / v->total_game) {
                average = v->total_win / v->total_game;
                index = i;
            }
        }
    }

    delete s;
    s = new Position(next_pos[index]);
    return s;
}

int main(int argc, char **argv) {
    srand(time(0));

    Position *s = new Position();
    double times1[2];
    double times2[2];
    int round_num = 0;

    int playout_num, iteration, thread_num;

    if (argc != 4) {
        cout << "usage: <iteration> <playout_num> <thread>" << endl;
        return 0;
    }

    iteration = atoi(argv[1]);
    playout_num = atoi(argv[2]);
    thread_num = atoi(argv[3]);
    omp_set_num_threads(thread_num);

    timing(times1, times1 + 1);
    while (!s->game_over()) {
        // cout << "----- round: " << round_num << " ------" << endl;
        s = mcts_play(s, iteration, playout_num, thread_num);
        s->print();
        round_num += 1;
        if (s->game_over())
            break;
        random_play(s);
        s->print();
    }
    timing(times2, times2 + 1);

    // cout << "Average time of one step: " << (times2[0] - times1[0]) / round_num << "s." << endl;

    delete s;
    return 0;
}
