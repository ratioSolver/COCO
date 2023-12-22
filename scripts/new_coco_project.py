import sys
import os

# Get the project name from the user
project_name = sys.argv[1]

print(f"Creating new project: {project_name}")

script_path = os.path.dirname(os.path.abspath(__file__))

app_header_template_path = os.path.join(script_path, "coco_app_hpp.tmpl")
db_header_template_path = os.path.join(script_path, "coco_db_hpp.tmpl")
listener_header_template_path = os.path.join(script_path, "coco_listener_hpp.tmpl")

app_src_template_path = os.path.join(script_path, "coco_app_cpp.tmpl")
db_src_template_path = os.path.join(script_path, "coco_db_cpp.tmpl")

# Create the project include directory
if not os.path.exists("include"):
    os.mkdir("include")

with open(app_header_template_path, 'r') as input_file:
    app_header_content = input_file.read()
    app_header_content = app_header_content.replace("[project_name]", project_name)

with open(f"include/{project_name}_app.hpp", "w") as f:
    f.write(app_header_content)

with open(db_header_template_path, 'r') as input_file:
    db_header_content = input_file.read()
    db_header_content = db_header_content.replace("[project_name]", project_name)

with open(f"include/{project_name}_db.hpp", "w") as f:
    f.write(db_header_content)

with open(listener_header_template_path, 'r') as input_file:
    listener_header_content = input_file.read()
    listener_header_content = listener_header_content.replace("[project_name]", project_name)

with open(f"include/{project_name}_listener.hpp", "w") as f:
    f.write(listener_header_content)

# Create the project source directory
if not os.path.exists("src"):
    os.mkdir("src")

with open(app_src_template_path, 'r') as input_file:
    app_src_content = input_file.read()
    app_src_content = app_src_content.replace("[project_name]", project_name)

with open(f"src/{project_name}_app.cpp", "w") as f:
    f.write(app_src_content)

with open(db_src_template_path, 'r') as input_file:
    db_src_content = input_file.read()
    db_src_content = db_src_content.replace("[project_name]", project_name)

with open(f"src/{project_name}_db.cpp", "w") as f:
    f.write(db_src_content)

print("Done!")