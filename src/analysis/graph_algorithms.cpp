#include "binaryatlas/analysis/graph_algorithms.hpp"

#include <algorithm>
#include <functional>
#include <stack>
#include <unordered_set>

namespace binaryatlas
{

std::vector<Address> depthFirstReachable(const AdjacencyList& adjacency, Address start)
{
  std::vector<Address> order;
  std::unordered_set<Address> visited;
  std::vector<Address> stack{start};

  while (!stack.empty())
  {
    const Address node = stack.back();
    stack.pop_back();
    if (!visited.insert(node).second)
    {
      continue;
    }

    order.push_back(node);
    const auto iterator = adjacency.find(node);
    if (iterator == adjacency.end())
    {
      continue;
    }

    for (auto edge = iterator->second.rbegin(); edge != iterator->second.rend(); ++edge)
    {
      stack.push_back(*edge);
    }
  }

  return order;
}

StronglyConnectedComponents tarjanStronglyConnectedComponents(const AdjacencyList& adjacency)
{
  StronglyConnectedComponents result;
  std::unordered_map<Address, std::size_t> indices;
  std::unordered_map<Address, std::size_t> lowlinks;
  std::unordered_set<Address> on_stack;
  std::vector<Address> stack;
  std::size_t next_index = 0;

  std::function<void(Address)> strong_connect = [&](Address node)
  {
    indices[node] = next_index;
    lowlinks[node] = next_index;
    ++next_index;
    stack.push_back(node);
    on_stack.insert(node);

    const auto iterator = adjacency.find(node);
    if (iterator != adjacency.end())
    {
      for (Address successor : iterator->second)
      {
        if (!indices.contains(successor))
        {
          strong_connect(successor);
          lowlinks[node] = std::min(lowlinks[node], lowlinks[successor]);
        }
        else if (on_stack.contains(successor))
        {
          lowlinks[node] = std::min(lowlinks[node], indices[successor]);
        }
      }
    }

    if (lowlinks[node] == indices[node])
    {
      std::vector<Address> component;
      while (!stack.empty())
      {
        const Address current = stack.back();
        stack.pop_back();
        on_stack.erase(current);
        component.push_back(current);
        if (current == node)
        {
          break;
        }
      }
      std::sort(component.begin(), component.end());
      result.components.push_back(std::move(component));
    }
  };

  std::vector<Address> nodes;
  nodes.reserve(adjacency.size());
  for (const auto& [node, _] : adjacency)
  {
    nodes.push_back(node);
  }
  std::sort(nodes.begin(), nodes.end());

  for (Address node : nodes)
  {
    if (!indices.contains(node))
    {
      strong_connect(node);
    }
  }

  std::sort(result.components.begin(), result.components.end(),
            [](const std::vector<Address>& left, const std::vector<Address>& right)
            { return left.front() < right.front(); });
  return result;
}

std::size_t connectedComponentCount(const AdjacencyList& adjacency)
{
  std::unordered_map<Address, std::vector<Address>> undirected;
  for (const auto& [node, edges] : adjacency)
  {
    undirected[node];
    for (Address edge : edges)
    {
      undirected[node].push_back(edge);
      undirected[edge].push_back(node);
    }
  }

  std::vector<Address> nodes;
  nodes.reserve(undirected.size());
  for (const auto& [node, _] : undirected)
  {
    nodes.push_back(node);
  }
  std::sort(nodes.begin(), nodes.end());

  std::unordered_set<Address> visited;
  std::size_t component_count = 0;
  for (Address node : nodes)
  {
    if (!visited.insert(node).second)
    {
      continue;
    }

    ++component_count;
    std::vector<Address> stack{node};
    while (!stack.empty())
    {
      const Address current = stack.back();
      stack.pop_back();
      for (Address successor : undirected[current])
      {
        if (visited.insert(successor).second)
        {
          stack.push_back(successor);
        }
      }
    }
  }

  return component_count;
}

} // namespace binaryatlas
