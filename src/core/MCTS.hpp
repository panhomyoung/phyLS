#pragma once

#include <cstdint>
#include <limits>
#include <iostream>
#include <unordered_map>
#include <assert.h>
#include <vector>
#include <random>
#include <cmath>

namespace MCTS{

class SimpleAction
{
public:
  static const size_t action_size { 3 };
  enum Action{
    delta_delay,
    delta_area,
    delta_wirelength
  };
  Action get_action(int const& choice)
  {
    if (choice < 0 || choice >= action_size) 
    {
      std::cerr << "Exceeded action amount boundary\n";
      throw std::out_of_range("Invalid action choice");
    }
    switch (choice)
    {
    case 0:
      return Action::delta_delay;
    case 1:
      return Action::delta_area;    
    case 2:
      return Action::delta_wirelength;
    default:{
      std::cerr<<"axceed action amount boundry\n";
      exit(0);
    } 
    }
  }
};

struct State
{
  // long term effect
  double Q {0};
  // exploration set to encourage exploration
  double P {1};
  // reward
  int R {0};
  // blocked if the node is being visited in parallel
  int B {0};
  // reward obtained from the implement
  double reward{0};
  // blocked child
  int blocked_child {0};
  // if the node is visited, it's 1, vice versus
  int visited {0};
  // if the node is the root node
  bool is_root {false};
  // if search should be terminated
  bool terminal_flag {false};

  // position of the state
  std::vector<State> children;
  // father node
  State* father { nullptr };
  // index of the node in state network
  std::string position = "R";

  bool terminal()
  {
    return terminal_flag;
  }
};

struct MCTS_params
{
  uint32_t search_iteration { 10 };
  uint32_t level_search { 15 };
  uint32_t sequence_length { 10 };
  uint32_t action_size { 4 };
  double PUCT { 50 };
  double lambda { 0.4 };
};

/*  The input implement should has following funtion:
      reward = take_action(int action) --> with given action, take action and return a reward
      terminal() --> if the agent should be terminated 
    The Position should has following function:
*/
template<typename Implement>
class MCTS_impl
{
  using DictState = std::unordered_map<std::string, State*>;
  using VecSequence = std::vector<char>;
public: 
  explicit MCTS_impl(Implement& MCTS_imple, MCTS_params const& ps): imple(MCTS_imple), param(ps), \
                                                action_size(ps.action_size), search_length_limit(param.sequence_length)
  {
    initialize_MCTS();
  };

  void run()
  {
    // std::cout<<"run\n";
    State* node_ptr = &root;

    for ( uint32_t i = 0; i < param.search_iteration; i++ )
    {
      // std::cout << "Iteration " << i << std::endl;
      root.visited++;
      auto prt = iter(root);

      // std::cout << "Current state: " << imple.get_state() << ", Reward: " << imple.get_reward() << std::endl;
    }
  }

  Implement* get()
  {
    if ( !imple.terminal()) {
      std::cerr<<"implement has not terminated!\n";
    }
    return &imple;
  }
private:
  // Initialize nodes in MCTS
  void initialize_MCTS()
  {
    // std::cout<<"initialize_mcts\n";
    root.is_root = true;
    init_node(root);
    root.visited++;
    auto baseline = imple.initialize();
    root.reward = baseline;
  }

  // produce a gaussian noise distributed by normal distribution
  double gaussian_rand(double m = 0.0, double v = 1)
  {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::normal_distribution<double> gaussian(m, v);

    return gaussian(gen);
  }

  void init_node(State& node)
  {
    // std::cout<<"action_size : "<<action_size<<"\n";
    for (int i = 0; i < action_size; i++)
    {
      State& child = node.children.emplace_back();
      child.father = &node;
      child.position = node.position + std::to_string(i);
      child.P = param.PUCT * (0.98 * 1/action_size + 0.02 * gaussian_rand(0.0, 1.0));
    }
  }

  double getValue(State const& node)
  {
    return (node.R + node.Q + get_U(node));
  }

  int bestAction(State const& node)
  {
    double best_value = std::numeric_limits<double>::lowest();
    int best_action = -1;
    int crt = -1;
    for (auto& child : node.children)
    {
      crt++;
      double value = getValue(child);

      if ( value > best_value )
      {
        best_value = value;
        best_action = crt;
      }
    }
    return best_action;
  }

  void simulate(State& node)
  { 
    // generate a seed of random integer
    init_node(node);
  }

  State* iter(State& node)
  {
    if (iteration > 0) imple.reinit();
    // std::cout<<"iteration : "<<iteration<<"\n";
    int i = 0;
    State* res = &node;
    State* r;
    while (!r->terminal() && i < search_length_limit)
    {
      r = res;
      res = forward(*res, i);
      i++;
      // std::cout<<"search depth is : "<<i<<"\t";
      // if(r->terminal()) std::cout<<"the node is set terminated\n";
    } 

    while (!res->is_root) 
    {
      if (res->father == nullptr)
      {
        std::cerr<<"Father node is nullptr\n";
      }
      res = backpropagate(*res);
    }     

    iteration++;
    
    return res;
  }

  // output final result of the implement
  

  // search forward for aquiring reward
  State* forward(State& node, int depth = 0)
  {
    // std::cout<<"searching position : "<<node.position<<"\n";
    /* the action is an integer */
    int action = bestAction(node);
    if (action < 0)
    {
      std::cerr << "failed to choose an action\n";
      throw std::out_of_range("Not select any choice");
    }
    // std::cout<<"\nThe best action is "<<action<<"\n";

    State& picked_child = node.children[action];
    std::string position = node.position + std::to_string(action);
    // expand a new node if it's unvisited
    if (!picked_child.visited)
    {
      if (state_ntk.find(position) == state_ntk.end())
      {
        init_node(picked_child);
        state_ntk[position] = &picked_child;
      }
      else
      {
        std::cerr << "node at position"<<picked_child.position
              <<" is unvisited but has been in state ntk\n";
        throw std::out_of_range("Invalid action choice");
      }
    }

    /* compute reward */
    double reward = imple.take_action(action, depth);

    imple.record_result(picked_child.position);
    if ( node.children[action].visited ) 
    { 
      // std::cout<<"Visited child's reward = "<<node.children[action].reward<<".\t"
      //   <<"Computed reward = "<<reward<<"\n"; 
      
    }
    else { 
      reward = imple.get_reward();
      double mulReward = imple.get_mulReward();
      if (reward < best_reward) { 
        best_reward = reward;
        imple.record_bestResult();
      }
      if (mulReward < best_mulReward)
      {
        best_mulReward = mulReward;
        imple.record_mulResult();
      }
    }
    // std::cout<<"reward : "<<reward<<"\n";
    picked_child.terminal_flag = imple.terminal();

    /* set explored child with parameters */
    picked_child.reward = reward;
    picked_child.R = get_R(picked_child);  
    picked_child.visited++;

    return &picked_child;
  }

  State* backpropagate(State& node)
  {
    State* father = node.father;
    if ( ((node.Q + node.R) * param.lambda) > father->Q )
    {
      father->Q = (node.Q + node.R) * param.lambda;
      // std::cout<<"updated father "<<father->position<<" to "<<father->Q<<"\n";
    }
    return father;
  }

  #pragma region compute statistic
  double get_R(State const& child)
  {
    double R;
    auto const& father = *(child.father);
    if (father.reward > child.reward)
    {
      R = (sqrt((father.reward - child.reward) / baseline));
    } 
    else{
      R = -1 * (sqrt((child.reward - father.reward) / baseline));
    }
    return R;
  }

  double get_U(State const& child)
  {
    double U;
    auto const& father = *(child.father);
    U = child.P * (sqrt(father.visited / (child.visited + 1)));
    return U;
  }

  #pragma endregion


private:
  Implement& imple;
  MCTS_params const& param;
  DictState state_ntk;
  VecSequence sequence;
  State root;
  
  /*  */
  int iteration {0};
  int action_size;
  int search_length_limit;
  double baseline {0.0};


  /* statistic */
  double best_reward = std::numeric_limits<double>::max();
  double best_mulReward = std::numeric_limits<double>::max();
};
} 