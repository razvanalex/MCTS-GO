#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mpi.h>
#include <omp.h>
#include <string>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unordered_map>
#include <vector>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>

using namespace std;

#define OMP_NUM_THREADS 2 // Don't beef it up too much

#define VISUAL              0   // For visualizing the game
#define LOG                 1   // For printing the average time

#define RANDOM_PLAY         0   // 1 = Random simulation

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
    friend class boost::serialization::access;

    template <class Archive>
    void serialize(Archive &ar, const unsigned int version) {

        ar &board;
        ar &is_visit;

        ar &player;

        ar &ko_row;
        ar &ko_col;
        ar &ko_turn;

        ar &win_player;
        ar &player1_pass;
        ar &player2_pass;
    }

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

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive &ar, const unsigned int version) {
        ar &last_pos_value;
        ar &total_game;
        ar &total_win;
    }

public:
    value *last_pos_value;
    double total_game;
    double total_win;

    value() {}

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

/* Total number of games, rounds and steps played */
unsigned long total_num_games = 0;
unsigned long total_num_rounds = 0;
unsigned long total_num_steps = 0;

/* The trees used in MCTS. It is a pair of (state, score) */
unordered_map<Position, value *> trees[2];
unordered_map<Position, value *> *localTrees;
unordered_map<Position, value *> ompTrees[OMP_NUM_THREADS];

/**
 * This is a generic function that broadcasts the data of a class that is 
 * serializable.
 */
template <class Type>
void broadcast(Type &data, int root) {
    string data_str;
    boost::iostreams::back_insert_device<std::string> data_inserter(data_str);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::string>> data_stream(data_inserter);
    boost::archive::binary_oarchive send_data(data_stream);

    send_data << data;
    data_stream.flush();

    int data_len = data_str.size();
    MPI_Bcast(&data_len, 1, MPI_INT, root, MPI_COMM_WORLD);

    char bcast_data[data_len];
    memcpy(bcast_data, data_str.c_str(), data_len);
    MPI_Bcast((void *)bcast_data, data_len, MPI_BYTE, root, MPI_COMM_WORLD);

    boost::iostreams::basic_array_source<char> device_data(bcast_data, data_len);
    boost::iostreams::stream<boost::iostreams::basic_array_source<char>> data_unpacker(device_data);
    boost::archive::binary_iarchive recv_data(data_unpacker);

    recv_data >> data;
}

/**
 * This is MCTS play.
 * @param s The current state.
 * @param iters The number of games played for simulation phase.
 * @param playout_num The number of simulations in MCTS.
 * @returns A new state after choosing a move.
 */
Position *mcts_play(Position *s, float iters, float playout_num, unordered_map<Position, value *> tree, int threadIndex, int thread_num) {
    int my_player = s->player;

    // The player cannot put a stone. Pass move!
    if (s->is_pass()) {
        s->pass_move();
        return s;
    }

    #pragma omp parallel  \
        reduction(+: total_num_games) \
        reduction(+: total_num_rounds) \
        reduction(+: total_num_steps)
    {
        int ompThreadIdx = omp_get_thread_num();
        unordered_map<Position, value *> localTree = tree;
        Position *s_local = new Position(*s);
    
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
            total_num_rounds++;

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
                    // Explore new state
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
                    total_num_games++;

                    while (!tt->game_over()) {
                        total_num_steps++; 
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
#if VISUAL
                            cout << "STUKED!" << endl;
                            tt->print();
#endif
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
                    } while (time2_sim - time1_sim < playout_num);
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
            delete t;

    #if USE_TIME_ROUND
            timing(&time2_round, &time_cpu);
        } while (time2_round - time1_round < iters);
    #else
        }
    #endif

    ompTrees[ompThreadIdx].clear();
    ompTrees[ompThreadIdx] = localTree;

    }

    for (int ompTID = 0; ompTID < OMP_NUM_THREADS; ompTID++) {
            if (threadIndex == 0) {
                localTrees[threadIndex * OMP_NUM_THREADS + ompTID] = ompTrees[ompTID];
        }

        if (threadIndex != 0) {
            string serial_str;

            boost::iostreams::back_insert_device<std::string> inserter(serial_str);
            boost::iostreams::stream<boost::iostreams::back_insert_device<std::string>> s(inserter);
            boost::archive::binary_oarchive send_ar(s);

            send_ar << ompTrees[ompTID];
            s.flush();

            int len = serial_str.size();

            int tag = threadIndex * OMP_NUM_THREADS + ompTID;
            MPI_Send(&len, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
            MPI_Send((void *)serial_str.data(), len, MPI_BYTE, 0, tag + 1, MPI_COMM_WORLD);

        } 
    }  

    if (threadIndex == 0) {
        for (int tid = 0; tid < thread_num; tid++) {
            if (tid == 0)
                continue;

            for (int ompTID = 0; ompTID < OMP_NUM_THREADS; ompTID++) {
                int tag = tid * OMP_NUM_THREADS + ompTID;
                int len = 0;

                MPI_Recv(&len, 1, MPI_INT, tid, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                char serial_str[len + 1];
                MPI_Recv(serial_str, len, MPI_BYTE, tid, tag + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                serial_str[len] = '\0';

                boost::iostreams::basic_array_source<char> device(serial_str, len);
                boost::iostreams::stream<boost::iostreams::basic_array_source<char>> s1(device);
                boost::archive::binary_iarchive recv_ar(s1);

                localTrees[tag].clear();
                recv_ar >> localTrees[tag];
            }
        }

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
    }

    // Broadcast the trees
    broadcast(tree, 0);

    if (threadIndex == 0) {
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

        delete s;
        s = new Position(next_pos[index]);
    }

    broadcast(s, 0);
    return s;
}

int main(int argc, char **argv) {
    srand(time(0));

    double times1[2];
    double times2[2];
    int round_num = 0;
    int thread_num;
    float playout_num, iteration;

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &thread_num);

    Position *s = new Position();

    omp_set_num_threads(OMP_NUM_THREADS);

    if (argc != 3) {
        cout << "usage: <iteration/time_round> <playout_num/time_sim>" << endl;
        return 0;
    }

#if USE_TIME_ROUND
    iteration = atof(argv[1]);
#else
    iteration = atoi(argv[1]);
#endif

#if USE_TIME_SIM
    playout_num = atof(argv[2]);
#else
    playout_num = atoi(argv[2]);
#endif

    localTrees = new unordered_map<Position, value *>[thread_num * OMP_NUM_THREADS];

    int threadIndex = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &threadIndex);

#if LOG
    if (threadIndex == 0)
        timing(times1, times1 + 1);
#endif

    while (!s->game_over()) {
#if VISUAL
        if (threadIndex == 0) {
            cout << endl
                 << "========= Round: " << round_num << " ==========" << endl;
            cout << "========== Player 1 ==========" << endl;
        }
#endif
        s = mcts_play(s, iteration, playout_num, trees[0], threadIndex, thread_num);

#if VISUAL
        if (threadIndex == 0)
            s->print();
#endif
        round_num += 1;
        if (s->game_over())
            break;

#if VISUAL
        if (threadIndex == 0)
            cout << "========== Player 2 ==========" << endl;
#endif
        s = mcts_play(s, iteration, playout_num, trees[1], threadIndex, thread_num);

#if VISUAL
        if (threadIndex == 0)
            s->print();
#endif
    }

#if LOG
    timing(times2, times2 + 1);

    unsigned long global_total_num_games = 0;
    unsigned long global_total_num_rounds = 0;
    unsigned long global_total_num_steps = 0;

    MPI_Reduce(&total_num_games, &global_total_num_games, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&total_num_rounds, &global_total_num_rounds, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&total_num_steps, &global_total_num_steps, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (threadIndex == 0) {
        cout << endl << "==== Game finished ====" << endl;
        cout << "Number of rounds played: " << round_num << endl;
        cout << "Total number of simulated games: " << global_total_num_games << " " << endl;
        cout << "Total number of simulated rounds: " << global_total_num_rounds << " " << endl;
        cout << "Total number of simulated steps: " << global_total_num_steps << " " << endl;
        cout << endl;
        cout << "Total time (seconds): " << times2[0] - times1[0] << endl;
        cout << "Average time for one round (seconds): " << (times2[0] - times1[0]) / round_num << endl;
        cout << endl;
    }
#endif

    // Release memory
    for (int i = 0; i < thread_num; i++) {
        auto tree = localTrees[i];
        for (auto it = tree.begin(); it != tree.end();) {
            delete it->second;
            it = tree.erase(it);
        }
    }
    delete[] localTrees;
    delete s;
    for (auto tree : trees) {
        for (auto it = tree.begin(); it != tree.end();) {
            delete it->second;
            it = tree.erase(it);
        }
    }
    
    MPI_Finalize();
    return 0;
}
