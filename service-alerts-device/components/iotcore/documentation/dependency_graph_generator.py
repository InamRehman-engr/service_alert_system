import os
import hashlib

def get_basename(filename):
    # Returns the basename of the file without extension
    return os.path.splitext(os.path.basename(filename))[0]

def generate_color(node):
    # Generates a color based on the node name for consistency
    hash_object = hashlib.md5(node.encode())
    return '#' + hash_object.hexdigest()[:6]

def convert_to_mermaid(graph_definition):
    lines = graph_definition.split("\n")
    mermaid_graph = "flowchart LR\n"  # Using Left-Right flowchart
    nodes = set()
    edges = []
    node_colors = {}

    # Process all nodes and edges
    for line in lines:
        if '->' in line or ('[' in line and 'label' in line):
            if '->' in line:
                source, target = line.split('->')
                source = get_basename(source.strip().strip('"'))
                target = get_basename(target.strip().strip('"').split('[')[0])
                nodes.add(source)
                nodes.add(target)
            else:
                node = get_basename(line.split('[')[0].strip().strip('"'))
                nodes.add(node)

    # Assign colors to nodes
    for node in nodes:
        node_colors[node] = generate_color(node)

    # Create edges and record their styles
    for line in lines:
        if '->' in line:
            source, target = line.split('->')
            source = get_basename(source.strip().strip('"'))
            target = get_basename(target.strip().strip('"').split('[')[0])

            # Avoid self-pointing dependencies
            if source != target:
                edge = f'{source} --> {target}'
                edges.append((source, edge))

    # Add edges and their styles to Mermaid graph
    for i, (source, edge) in enumerate(edges):
        mermaid_graph += f'    {edge}\n'
        mermaid_graph += f'    linkStyle {i} stroke:{node_colors[source]},stroke-width:2px;\n'

    return mermaid_graph

# Read graph definition from a file
input_file_path = 'codeviz.dot'  # Update this to your input file path
with open(input_file_path, 'r') as file:
    graph_definition = file.read()

# Convert to Mermaid graph
mermaid_graph = convert_to_mermaid(graph_definition)

# Write output to a Markdown file
output_file_path = 'Dependency Graph.md'  # Update this to your output file path
with open(output_file_path, 'w') as file:
    file.write("```mermaid\n" + mermaid_graph + "```\n")

print(f"Mermaid graph written to {output_file_path}")
