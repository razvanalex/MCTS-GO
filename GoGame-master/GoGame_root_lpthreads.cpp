#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include <pthread.h>
#include <string.h>

using namespace std;

#define VISUAL              1   // For visualizing the game
#define LOG                 0   // For printing the average time

#define RANDOM_PLAY         1   // 1 = Random simulation

#define USE_TIME_ROUND      1   // 1 = Use time; 0 = Use iterations
#define TIME_PER_ROUND      0.5   // In seconds

#define USE_TIME_SIM        1   // 1 = Use time; 0 = Use iterations
#define TIME_PER_SIM        0.05 // In seconds


/**
 * Measures the time.
 * @param wcTime The time measured on world clock
 * @param cpuTime The time measured on CPU
 */
void timing(double *wcTime, double *cpuTime) {
    struct timeval tp;
    struct rusage ruse;

    gettimeofday(&tp, NULL);
    *wcTime = (double)(tp.tv_sec + tp.tv_usec / 1000000.0);

    getrusage(RUSAGE_SELF, &ruse);
    *cpuTime = (double)(ruse.ru_utime.tv_sec + ruse.ru_utime.tv_usec / 1000000.0);
}

/**
 * This class represents a node in the tree.
 */
class Position {
public:
    vector<vector<int>> board;
    vector<vector<bool>> is_visit;
    int player;
    int ko_row, ko_col, ko_turn;

    int win_player;
    bool player1_pass;
    bool player2_pass;

    /**
     * Create a new state of the game.
     */
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

    /**
     * Create a new copy of the state given as parameter.
     * @param t The state to be copied.
     */
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

    /**
     * Create a new state, given the board and the player.
     * @param board_ Current board state.
     * @param p Current player.
     */
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

    /**
     * Check if a move is valid.
     * @param row The row where to check if it is a valid move.
     * @param col The column where to check if it is a valid move.
     */
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

    /**
     * Make a move.
     * @param row The row where to put the stone.
     * @param col The column where to put the stone.
     */
    int make_move(int row, int col) {
        // pass for human player
        if (row == -2 && col == -2) {
            player = -player;
            ko_row = -1;
            ko_col = -1;
            return -2;
        }

        if (board[row][col] != 0 || (row == ko_row && col == ko_col)) {
            return -1;
        } else {
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

    /**
     * Check if current player passes the game.
     */
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

    /**
     * Change the current state, when the player cannot make a move.
     */
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

    /**
     * Check if the game is over.
     */
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
        }
        return player1_pass && player2_pass;
    }

    /**
     * Returns the winning player.
     */
    int who_win() {
        return win_player;
    }

    /**
     * Print the board in current state.
     */
    void print() {
        cout << "  0 1 2 3 4 5 6 7 8" << endl;
        for (int i = 0; i < 9; ++i) {
            cout << " " << i;
            for (int j = 0; j < 9; ++j) {
                if (board[i][j] == 1)
                    cout << "#"
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

    /**
     * Convert current state to a string.
     */
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

    /**
     * Compare two different states.
     * @param p The state to be compared with.
     */
    bool operator==(const Position &p) const {
        return (board == p.board) && (player == p.player);
    }

private:
    /**
     * Count the eyes using DFS, for a given player. A single empty space inside
     * a group is called an eye.
     * @param row The row where to check for eye.
     * @param col The column where to check for eye.
     * @param player_ The player for which to search of eye.
     */
    int count_eyes(int row, int col, int player_) {
        // Invalid parameters, return -1
        if (row < 0 || row > 8 || col < 0 || col > 8 || board[row][col] != player_)
            return -1;

        // Reset the state of is_visit
        for (int i = 0; i < 9; ++i) {
            for (int j = 0; j < 9; ++j) {
                is_visit[i][j] = false;
            }
        }

        // Apply DFS to count the eyes
        int res = 0;
        dfs(row, col, board[row][col], res);
        return res;
    }

    /**
     * Count the eyes.
     */
    void dfs(int row, int col, int player_, int &res) {
        // Invalid state or visited state or other player's stone at (row, col), finish the search
        if (row < 0 || row > 8 || col < 0 || col > 8 || is_visit[row][col] == true || board[row][col] == -player_)
            return;

        // Mark (row, col) as visited
        is_visit[row][col] = true;

        // If the (row, col) is of player_, continue searching on neighbors 
        if (board[row][col] == player_) {
            dfs(row + 1, col, player_, res);
            dfs(row - 1, col, player_, res);
            dfs(row, col + 1, player_, res);
            dfs(row, col - 1, player_, res);
        }

        // If (row, col) is 0 (empty cell), increment the result
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

    /**
     * Removes stones form the board, for a given player.
     */
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

/**
 * This class is a linked list of values for total games and total wins.
 */
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
#if VISUAL
        cout << total_game << " " << total_win << endl;
#endif
    }
};

/**
 * Creates a list of possible moves from current state. This list can be also
 * seen as the possible childs of current state.
 * @param s Current state.
 * @param next_pos An array of next states, returned by side effect.
 */
void get_next_pos(Position *s, vector<Position> &next_pos) {
    next_pos.clear();

    // If pass move, then the next pos is only the "pass move"
    if (s->is_pass()) {
        Position tmp(*s);
        tmp.pass_move();
        next_pos.push_back(tmp);
        return;
    }

    // Check for each cell if the move can be done.
    for (int i = 0; i < 9; ++i) {
        for (int j = 0; j < 9; ++j) {
            Position tmp(*s);

            if (tmp.make_move(i, j) != -1)
                next_pos.push_back(tmp);
        }
    }
}

namespace std {
    /* Overwrite the hash operation */
	template <> struct hash<Position> {
		size_t operator()(const Position obj) const {
			return hash<string>()(obj.to_string());
		}
	};
}

/**
 * This is a random player.
 * @param s The current state.
 */
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

/**
 * This is for manual play (playing against a human).
 * @param s The current state.
 */
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

/* The trees used in MCTS. It is a pair of (state, score) */
unordered_map<Position, value *> trees[2];
unordered_map<Position, value *> *localTrees;


struct parameters {
    unordered_map<Position, value *> tree;
    Position *s;

    int player;
    int playout_num;
    int threadId;
};

void* Explore_Arb(void *params) {
    struct parameters *p = (struct parameters *) params;

    unordered_map<Position, value *> localTree = p->tree;
    Position *s_local = new Position(*(p->s));

    int my_player= p->player;
    int playout_num = p->playout_num;
    int threadIndex = p->threadId;

    // If 's' (current state) is not in the tree, create the state
    if (localTree.find(*s_local) == localTree.end()) {
        // Create a linked-list for game scores
        localTree[*s_local] = new value(NULL, 0, 0);

        // Get the available moves
        vector<Position> next_pos;
        get_next_pos(s_local, next_pos);

        // For every move, check if the state exists in the tree then update
        // the scores for current state
        value *v = localTree[*s_local];
        for (int i = 0; i < next_pos.size(); ++i) {
            if (localTree.find(next_pos[i]) != localTree.end()) {
                v->total_game += localTree[next_pos[i]]->total_game;
                v->total_win += localTree[next_pos[i]]->total_win;
            }
        }
    }

    bool all_in;
    value *root_v = localTree[*s_local];
    Position *t;

    // Run the game 'iters' times
#if USE_TIME_ROUND
    double time1_round, time2_round, time_cpu;
    timing(&time1_round, &time_cpu);
    do {
#else
    #pragma omp for schedule(dynamic)
    for (int i = 0; i < iters; ++i) {
#endif
        t = s_local;

        // Selection
        while (!t->game_over()) {
            all_in = true; // all child nodes are expanded

            // Get next moves
            vector<Position> next_pos;
            get_next_pos(t, next_pos);

            // Search the first move from that is not in the existing tree.
            // Also, if one move not found, set all_in to false
            int index = 0;
            for (int j = 0; j < next_pos.size(); ++j) {
                if (localTree.find(next_pos[j]) == localTree.end()) {
                    all_in = false;
                    index = j;
                    break;
                }
            }

            // If all_in then tree policy else expand (create a new node) and break
            if (all_in == false) {
                // Explore new stateUSE_TIME
                localTree[next_pos[index]] = new value(localTree[*t], 0.0, 0.0);
                t = new Position(next_pos[index]);
                break;
            } else {
                // All child nodes are visited. Find the best current children,
                // UCTS strategy
                double z = 0.2;
                value *v = localTree[*t];
                double T = v->total_game;
                Position *tmp_pos = NULL;
                int best_row, best_col;

                // Select next child
                if (t->player == my_player) {
                    // For my_player, select the child that maximizes the score
                    double ucb = -10000000000000.0;
                    for (int j = 0; j < next_pos.size(); ++j) {
                        value *vv = localTree[next_pos[j]];
                        double pj = vv->total_win;
                        double nj = vv->total_game;
                        double tmp_ucb = pj / nj + sqrt(z * log(T) / nj);

                        if (ucb < tmp_ucb) {
                            ucb = tmp_ucb;
                            tmp_pos = &next_pos[j];
                        }
                    }
                } else {
                    // For the other player, select the child that minimizes the score
                    double ucb = 10000000000000.0;
                    for (int j = 0; j < next_pos.size(); ++j) {
                        value *vv = localTree[next_pos[j]];
                        double pj = vv->total_win;
                        double nj = vv->total_game;
                        double tmp_ucb = pj / nj - sqrt(z * log(T) / nj);

                        if (ucb > tmp_ucb) {
                            ucb = tmp_ucb;
                            tmp_pos = &next_pos[j];
                        }
                    }
                }

                // Add to linked-list, the parent value
                localTree[*tmp_pos]->last_pos_value = v;
                if (t != s_local)
                    delete t;

                // Go to next state
                t = new Position(*tmp_pos);
            }
        }

        // Playout policy: Run some random games and obtain some scores
        double total_g = 0, total_w = 0;
        if (t->game_over()) {
            // 't' is a final state, just update the scores
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
            // Run simulations 'playout_num' times
#if USE_TIME_SIM
            double time1_sim, time2_sim, time_cpu;
            timing(&time1_sim, &time_cpu);
            do {
#else
            for (int j = 0; j < playout_num; ++j) {
#endif
                Position *tt = new Position(*t);
                // Run a random simulation
                int steps = 0;

                while (!tt->game_over()) {
                    steps++;

                    if (!tt->is_pass()) {
                        int x = 0, y = 0;
#if RANDOM_PLAY
                        do {
                            x = rand() % 9;
                            y = rand() % 9;
                        } while (tt->make_move(x, y) == -1);
#else
                        do {
                            x++;
                            if (x == 9) {
                                x = 0;
                                y++;
                            }
                            y %= 9;
                        } while (tt->make_move(x, y) == -1);
#endif
                    } else {
                        tt->pass_move();
                    }

                    if (steps == 5000) {
                        cout << "STUKED!" << endl;
                        tt->print();
                        break;
                    }
                }

                // Update the scores based on the result of the last game
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
#if USE_TIME_SIM
                timing(&time2_sim, &time_cpu);
            } while (time2_sim - time1_sim < TIME_PER_SIM);
#else
            }
#endif
        }

        // Back propagate the result (up to 500 levels of the tree)
        value *v_ = localTree[*t];
        v_->total_game += total_g;
        v_->total_win += total_w;
        int loop_num = 0;

        while (v_ != root_v && loop_num++ < 500) {
            v_ = v_->last_pos_value;
            v_->total_game += total_g;
            v_->total_win += total_w;
        }

#if USE_TIME_ROUND
        timing(&time2_round, &time_cpu);
    } while (time2_round - time1_round < TIME_PER_ROUND);
#else
    }
#endif

    localTrees[threadIndex] = localTree;
}


struct job {
    struct parameters thread_param;
    void *(*job_func)(void *params);
    int thread_num;
};

pthread_cond_t wait_job = PTHREAD_COND_INITIALIZER;
pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t status_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;

struct job *global_params;
int global_count = 0;
enum status { RUNNING, FINISHED } status;


int submit_job(struct job *params) {
    global_count = params->thread_num;
    global_params = params;
    pthread_cond_broadcast(&wait_job);

    return 0;
}

struct job *get_job(int threadId) {
    if (global_count == 0)
        return NULL;

    struct job *new_job = (struct job *)malloc(sizeof(struct job));
    if (!new_job)
        return NULL;

    memcpy(new_job, global_params, sizeof(struct job));
    new_job->thread_param.threadId = threadId;

    return new_job;
}

void finish_jobs() {
    pthread_barrier_wait(&barrier);
    pthread_mutex_lock(&status_mutex);
    status = FINISHED;
    pthread_cond_broadcast(&wait_job);
    pthread_mutex_unlock(&status_mutex);
}

void *run_job(void *args) {
    int threadId = *(int *)args;
    struct job *my_job;
    struct parameters *_params;

    // Wait for all jobs to finish
    pthread_barrier_wait(&barrier);
    
    while (status == RUNNING) {
        my_job = NULL;

        // Get the job
        pthread_mutex_lock(&job_mutex);
        while ((my_job = get_job(threadId)) == NULL) {
            pthread_cond_wait(&wait_job, &job_mutex);
            if (status == FINISHED)
                break;
        }
        pthread_mutex_unlock(&job_mutex);
        
        // Don't run the job is the status is FINISHED
        if (status == FINISHED)
            break;

        // Run the job
        my_job->job_func(&my_job->thread_param);
        free(my_job);

        // Wait for all jobs to finish
        pthread_barrier_wait(&barrier);

    }

    return NULL;
}

/**
 * This is MCTS play.
 * @param s The current state.
 * @param iters The number of games played for simulation phase.
 * @param playout_num The number of simulations in MCTS.
 * @returns A new state after choosing a move.
 */
Position *mcts_play(Position *s, int iters, int playout_num, unordered_map<Position, value *> tree, int thread_num) {
    int my_player = s->player;

    // The player cannot put a stone. Pass move!
    if (s->is_pass()) {
        s->pass_move();
        return s;
    }

    struct job scheduled_job;

    // Prepare the job
    scheduled_job.job_func = &Explore_Arb;
    scheduled_job.thread_num = thread_num;
    scheduled_job.thread_param.tree = tree;
    scheduled_job.thread_param.s = new Position(*s);
    scheduled_job.thread_param.player = my_player;
    scheduled_job.thread_param.playout_num = playout_num;
    scheduled_job.thread_param.threadId = 0;

    // Submit and run
    submit_job(&scheduled_job);

    struct job *my_job = get_job(0);
    if (!my_job) {
        fprintf(stderr, "ERROR! Master thread should get a job.\n");
        exit(-1);
    }

    // Run the job
    my_job->job_func(&my_job->thread_param);
    free(my_job);

    // Wait for all jobs to finish
    pthread_barrier_wait(&barrier);

    // Merge local trees to global trees
    for (int i = 0; i < thread_num; ++i) {
        for (auto node = localTrees[i].begin(); node != localTrees[i].end(); node++) {
            if (tree.find(node->first) == tree.end() && node->second->total_game != 0) {
                tree[node->first] = node->second;
            } else if (node->second->total_game != 0) {
                tree[node->first]->total_game += node->second->total_game;
                tree[node->first]->total_win += node->second->total_win;
            }
        }
    }

    // Choose the best move
    vector<Position> next_pos;
    get_next_pos(s, next_pos);
    double average = -10.0;
    int index = -1;

    for (int i = 0; i < next_pos.size(); ++i) {
        if (tree.find(next_pos[i]) != tree.end()) {
            value *v = tree[next_pos[i]];
            if (average < v->total_win / v->total_game) {
                average = v->total_win / v->total_game;
                index = i;
            }
        }
    }

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
        cout << "usage: <iteration> <playout_num> <num_threads>" << endl;
        return 0;
    }

    iteration = atoi(argv[1]);
    playout_num = atoi(argv[2]);
    thread_num = atoi(argv[3]);

    localTrees = new unordered_map<Position, value *>[thread_num];
    
    // Init the barrier
    pthread_barrier_init(&barrier, NULL, thread_num);

    // Create the thread pool
    pthread_t *threads = NULL;
    int *tid = NULL;

    if (thread_num > 1) {
        threads = (pthread_t *)malloc((thread_num - 1) * sizeof(pthread_t));
        tid = (int *)malloc((thread_num) * sizeof(int));
        status = RUNNING;
        for (int i = 1; i < thread_num; i++) {
            tid[i] = i;
            pthread_create(&threads[i], NULL, run_job, &tid[i]);
        }    
    }

#if LOG
    timing(times1, times1 + 1);
#endif

    // Wait for all jobs to finish
    pthread_barrier_wait(&barrier);

    while (!s->game_over()) {
#if VISUAL
        cout << endl
             << "========= Round: " << round_num << " ==========" << endl;
        cout << "========== Player 1 ==========" << endl;
#endif
        s = mcts_play(s, iteration, playout_num, trees[0], thread_num);

#if VISUAL
        s->print();
#endif
        round_num += 1;
        if (s->game_over())
            break;

#if VISUAL
        cout << "========== Player 2 ==========" << endl;
#endif
        s = mcts_play(s, iteration, playout_num, trees[1], thread_num);

#if VISUAL
        s->print();
#endif
    }
 
#if LOG
    timing(times2, times2 + 1);
    cout << "Average time of one step: " << (times2[0] - times1[0]) / round_num << "s." << endl;
#endif

    // Join all threads created
    if (thread_num > 1) {
        finish_jobs();
        for (int i = 1; i < thread_num; i++)
            pthread_join(threads[i], NULL);
        free(threads);
        free(tid);
    }

    // Destroy mutexes, conditions, barriers etc
    pthread_cond_destroy(&wait_job);
    pthread_mutex_destroy(&job_mutex);
    pthread_barrier_destroy(&barrier);

    // Release memory
    delete[] localTrees;
    delete s;

    return 0;
}
