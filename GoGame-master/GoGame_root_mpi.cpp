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
#include <mpi.h>
#include <fstream>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/serialization/unordered_map.hpp>

#include <boost/iostreams/device/back_inserter.hpp>

#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/list.hpp>


using namespace std;

ofstream f[4];

#define VISUAL 1    // For visualizing the game
#define LOG 0       // For printing the average time

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

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {

        ar & board;
        ar & is_visit;

        ar & player;
        
        ar & ko_row;
        ar & ko_col;
        ar & ko_turn;

        ar & win_player;
        ar & player1_pass;
        ar & player2_pass;
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

    friend class  boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & last_pos_value;
        ar & total_game;
        ar & total_win;
    }

public:
    value *last_pos_value;
    double total_game;
    double total_win;

    value(){}

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


int game_over = 0;

/**
 * This is MCTS play.
 * @param s The current state.
 * @param iters The number of games played for simulation phase.
 * @param playout_num The number of simulations in MCTS.
 * @returns A new state after choosing a move.
 */
Position *mcts_play(Position *s, int iters, int playout_num, unordered_map<Position, value *> tree, int threadIndex, int thread_num) {
    int my_player = s->player;

    if (threadIndex != 0) {
        MPI_Bcast(&game_over, 0, MPI_INT, 0, MPI_COMM_WORLD);
        f[threadIndex] << threadIndex <<  " " << game_over << ": =======Broadcast==========" << endl;
        // if (game_over) {
        //     return NULL;
        // }

    } else {
        cout << game_over << endl;
        // The player cannot put a stone. Pass move!
        if (game_over || s->is_pass()) {
            s->pass_move();

            game_over = 1;
            
            MPI_Bcast(&game_over, 0, MPI_INT, 0, MPI_COMM_WORLD);
           
            f[threadIndex] << threadIndex <<  " " << game_over << ": =======Broadcast==========" << endl;

            cout << game_over << endl;
        } else {

            MPI_Bcast(&game_over, 0, MPI_INT, 0, MPI_COMM_WORLD);
            cout << game_over << endl;
        }
        f[threadIndex] << threadIndex << ": =======HH==========" << endl;
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    game_over = 1447;
    MPI_Bcast(&game_over, 0, MPI_INT, 0, MPI_COMM_WORLD);
    f[threadIndex] << game_over << ": ======= GAME OVER ==========" << endl;

    if (game_over)
        return s;

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
    for (int i = 0; i < iters; ++i) {
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
            for (int j = 0; j < playout_num; ++j) {
                Position *tt = new Position(*t);
                // Run a random simulation
                int steps = 0;

                while (!tt->game_over()) {
                    steps++;

                    if (!tt->is_pass()) {
                        int x = 0, y = 0;
                        do {
                            x = rand() % 9;
                            y = rand() % 9;
                        } while (tt->make_move(x, y) == -1);
                        // do {
                        //     x++;
                        //     if (x == 9) {
                        //         x = 0;
                        //         y++;
                        //     }
                        //     y %= 9;
                        // } while (tt->make_move(x, y) == -1);
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
            }
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
    }
    f[threadIndex] << "1" << endl;

    if (threadIndex != 0) {
        string serial_str;
        boost::iostreams::back_insert_device<std::string> inserter(serial_str);
        boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > s(inserter);
        boost::archive::binary_oarchive send_ar(s);

        send_ar << localTree;
        s.flush();

        int len = serial_str.size();

        MPI_Send(&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send((void *)serial_str.data(), len, MPI_BYTE, 0, 1, MPI_COMM_WORLD);
    
    } else {
        localTrees[threadIndex] = localTree;
    }

    if (threadIndex == 0) {
        for (int tid = 0; tid < thread_num; tid++) {
            if  (tid == 0)
                continue;

            int len;
            MPI_Recv(&len, 1, MPI_INT, tid, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            char serial_str[len + 1];
            MPI_Recv(serial_str, len, MPI_BYTE, tid, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            serial_str[len] = '\0';

            boost::iostreams::basic_array_source<char> device(serial_str, len);
            boost::iostreams::stream<boost::iostreams::basic_array_source<char> > s1(device);
            boost::archive::binary_iarchive recv_ar(s1);

            localTrees[tid].clear();
            recv_ar >> localTrees[tid];     
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

    f[threadIndex] << "2" << endl;

    // TODO: Broadcast the trees
    string tree_str;
    boost::iostreams::back_insert_device<std::string> tree_inserter(tree_str);
    boost::iostreams::stream<boost::iostreams::back_insert_device<std::string> > tree_stream(tree_inserter);
    boost::archive::binary_oarchive send_tree(tree_stream);

    send_tree << tree;
    tree_stream.flush();

    int tree_len = tree_str.size();


    MPI_Bcast(&tree_len, 1, MPI_INT, 0, MPI_COMM_WORLD);
    f[threadIndex] << "3" << endl;

    char tree_data[tree_len];
    memcpy(tree_data, tree_str.c_str(), tree_len);

    MPI_Bcast((void *)tree_data, tree_len, MPI_BYTE, 0, MPI_COMM_WORLD);
    f[threadIndex] << "4" << endl;

    boost::iostreams::basic_array_source<char> device_tree(tree_data, tree_len);
    boost::iostreams::stream<boost::iostreams::basic_array_source<char> > tree_unpacker(device_tree);
    boost::archive::binary_iarchive recv_tree(tree_unpacker);

    recv_tree >> tree;


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
        return s;
    }
    
    return NULL;
}

int main(int argc, char **argv) {
    srand(time(0));
    char name[30];
    for (int i = 0; i < sizeof(f) / sizeof(f[0]); i++) {
        sprintf(name, "out_%d.txt", i);
        f[i].open(name);
    }

    Position *s_tmp;
    Position *s = new Position();
    double times1[2];
    double times2[2];
    int round_num = 0;
    int playout_num, iteration, thread_num;

    if (argc != 3) {
        cout << "usage: <iteration> <playout_num>" << endl;
        return 0;
    }

    iteration = atoi(argv[1]);
    playout_num = atoi(argv[2]);

    /* Initialize MPI */
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &thread_num);
    
    localTrees = new unordered_map<Position, value *>[thread_num];
    
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
        f[threadIndex] << threadIndex << ": ============IN=====" << endl;
        game_over = 0;
        MPI_Barrier(MPI_COMM_WORLD);
        s_tmp = mcts_play(s, iteration, playout_num, trees[0], threadIndex, thread_num);
        if (threadIndex == 0)
            s = s_tmp;

    if (threadIndex == 0) {
#if VISUAL
        s->print();
#endif
        round_num += 1;
        if (s->game_over())
            break;
    
#if VISUAL
        cout << "========== Player 2 ==========" << endl;
#endif
    }
        game_over = 0;

        MPI_Barrier(MPI_COMM_WORLD);
        s_tmp = mcts_play(s, iteration, playout_num, trees[1], threadIndex, thread_num);
        if (threadIndex == 0)
            s = s_tmp;

        f[threadIndex] << threadIndex << ": =================" << endl;
#if VISUAL
        if (threadIndex == 0)
            s->print();
#endif
    }
    
    cout << "final" << endl;
 
#if LOG
    if (threadIndex == 0) {
        timing(times2, times2 + 1);
        cout << "Average time of one step: " << (times2[0] - times1[0]) / round_num << "s." << endl;
    }
#endif

    // if (threadIndex == 0) {

    // }

    MPI_Finalize();

    // Release memory
    delete[] localTrees;
    delete s;
    for (int i = 0; i < sizeof(f) / sizeof(f[0]); i++)
        f[i].close();
    
    return 0;
}
