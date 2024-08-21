import sys
import os

def create_project() -> int:
    script_path = os.path.dirname(os.path.abspath(__file__))

    # Ask the user for the project name
    project_name = input('Enter the name of your new project: ')

    # Ask the user if the project should include an API
    include_api = input('Should the project include an API? (yes/no): ').strip().lower()
    if include_api == 'yes' or include_api == 'y':
        include_api = True
    else:
        include_api = False

    # Ask the user if the project should include a custom database
    include_db = input('Should the project include a custom database? (yes/no): ').strip().lower()
    if include_db == 'yes' or include_db == 'y':
        include_db = True
    else:
        include_db = False

    # Create the project directory
    project_dir = os.path.join(script_path, project_name)
    os.mkdir(project_dir)

    # Create the include directory
    include_dir = os.path.join(project_dir, 'include')
    os.mkdir(include_dir)

    # Create the src directory
    src_dir = os.path.join(project_dir, 'src')
    os.mkdir(src_dir)

    # Create the project header file
    project_header_path = os.path.join(include_dir, f'{project_name}.hpp')
    with open(project_header_path, 'w') as f:
        f.write(f'#pragma once\n\n')
        if include_api:
            f.write(f'#include "coco_server.hpp"\n\n')
        else:
            f.write(f'#include "coco_core.hpp"\n\n')
        f.write(f'namespace {project_name} {{\n\n')
        if include_api:
            f.write(f'  class {project_name} : public coco::coco_server {{\n')
        else:
            f.write(f'  class {project_name} : public coco::coco_core {{\n')
        f.write(f'  public:\n')
        f.write(f'    {project_name}();\n\n')
        f.write(f'  }};\n')
        f.write(f'}} // namespace {project_name}\n')

    if include_db:
        # Create the database header file
        db_header_path = os.path.join(include_dir, f'{project_name}_db.hpp')
        with open(db_header_path, 'w') as f:
            f.write(f'#pragma once\n\n')
            f.write(f'#include "mongo_db.hpp"\n\n')
            f.write(f'namespace {project_name} {{\n\n')
            f.write(f'  class {project_name}_db : public coco::mongo_db {{\n')
            f.write(f'  public:\n')
            f.write(f'    {project_name}_db();\n\n')
            f.write(f'  }};\n')
            f.write(f'}} // namespace {project_name}\n')

    # Create the project source file
    project_source_path = os.path.join(src_dir, f'{project_name}.cpp')
    with open(project_source_path, 'w') as f:
        f.write(f'#include "{project_name}.hpp"\n\n')
        if include_db:
            f.write(f'#include "{project_name}_db.hpp"\n\n')

        f.write(f'namepace {project_name} {{\n\n')
        if include_api:
            if include_db:
                f.write(f'  {project_name}::{project_name}() : coco_server(std::make_unique<{project_name}_db>()) {{\n')
            else:
                f.write(f'  {project_name}::{project_name}() : coco_server() {{\n')
        else:
            if include_db:
                f.write(f'  {project_name}::{project_name}() : coco_core(std::make_unique<{project_name}_db>()) {{\n')
            else:
                f.write(f'  {project_name}::{project_name}() : coco_core() {{\n')
        f.write(f'  }}\n')
        f.write(f'}} // namespace {project_name}\n')

    if include_db:
        # Create the database source file
        db_source_path = os.path.join(src_dir, f'{project_name}_db.cpp')
        with open(db_source_path, 'w') as f:
            f.write(f'#include "{project_name}_db.hpp"\n\n')
            f.write(f'namespace {project_name} {{\n\n')
            f.write(f'  {project_name}_db::{project_name}_db() : mongo_db() {{\n')
            f.write(f'  }}\n')
            f.write(f'}} // namespace {project_name}\n')

    # Create the CMakeLists.txt file
    cmake_path = os.path.join(project_dir, 'CMakeLists.txt')
    with open(cmake_path, 'w') as f:
        f.write(f'cmake_minimum_required(VERSION 3.10)\n')
        f.write(f'project({project_name})\n')
        f.write(f'set(CMAKE_CXX_STANDARD 17)\n\n')
        f.write(f'set(COCO_NAME "{project_name}" CACHE STRING "The CoCo project name")\n')
        f.write(f'add_executable({project_name} src/{project_name}.cpp')
        if include_db:
            f.write(f' src/{project_name}_db.cpp')
        f.write(f')\n')
        f.write(f'target_include_directories({project_name} PRIVATE include)\n')
        f.write(f'target_link_libraries({project_name} CoCo)\n')

    return 0

if __name__ == '__main__':
    sys.exit(create_project())
