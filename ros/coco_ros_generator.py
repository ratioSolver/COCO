#!/usr/bin/env python3
import argparse
import json
import sys
import pathlib
from typing import Dict, Any, List, Iterable

def _collect_type_files(files: Iterable[str], folders: Iterable[str]) -> Iterable[pathlib.Path]:
    collected = [pathlib.Path(entry) for entry in files]
    for folder in folders:
        directory = pathlib.Path(folder)
        if not directory.is_dir():
            continue
        for entry in sorted(directory.iterdir()):
            if entry.is_file():
                collected.append(entry)
    return collected

def _load_types(type_paths: Iterable[pathlib.Path]) -> Dict[str, Any]:
    types: Dict[str, Any] = {}
    for path in type_paths:
        with path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
        name = data["name"]
        types[name] = data
    return types

def _create_project_structure(output_dir: pathlib.Path) -> None:
    base_dir = output_dir / "src" / "coco_ros"
    base_dir.mkdir(parents=True, exist_ok=True)

def _write_header(output_dir: pathlib.Path, types: Dict[str, Any]) -> None:
    header_path = output_dir / "src" / "coco_ros" / "coco_ros.hpp"
    with header_path.open("w", encoding="utf-8") as handle:
        handle.write("#pragma once\n\n")
        handle.write("#include \"rclcpp/rclcpp.hpp\"\n\n")

def _write_source(output_dir: pathlib.Path, types: Dict[str, Any]) -> None:
    source_path = output_dir / "src" / "coco_ros" / "coco_ros.cpp"
    with source_path.open("w", encoding="utf-8") as handle:
        handle.write("#include \"coco_ros/coco_ros.hpp\"\n\n")

def _write_package_xml(output_dir: pathlib.Path) -> None:
    pkg_path = output_dir / "src" / "coco_ros" / "package.xml"
    with pkg_path.open("w", encoding="utf-8") as handle:
        handle.write("<?xml version=\"1.0\"?>\n")
        handle.write("<?xml-model href=\"http://download.ros.org/schema/package_format3.xsd\" schematypens=\"http://www.w3.org/2001/XMLSchema\"?>")
        handle.write("<package format=\"3\">\n")
        handle.write("  <name>coco_ros</name>\n")
        handle.write("  <version>0.1.0</version>\n")
        handle.write("  <description>CoCo ROS</description>\n")
        handle.write("  <maintainer email=\"riccardo.debenedictis@cnr.it\">Riccardo De Benedictis</maintainer>\n")
        handle.write("  <license>Apache-2.0</license>\n")
        handle.write("  <buildtool_depend>ament_cmake</buildtool_depend>\n")
        handle.write("</package>\n")

def _write_cmake_lists(output_dir: pathlib.Path) -> None:
    cmake_path = output_dir / "src" / "coco_ros" / "CMakeLists.txt"
    with cmake_path.open("w", encoding="utf-8") as handle:
        handle.write("cmake_minimum_required(VERSION 3.5)\n")
        handle.write("project(coco_ros)\n\n")
        handle.write("find_package(ament_cmake REQUIRED)\n")
        handle.write("find_package(rclcpp REQUIRED)\n\n")
        handle.write("ament_package()\n")

def main() -> int:
    parser = argparse.ArgumentParser(description="Generate CoCo ROS package")
    parser.add_argument("-t", "--type-files", nargs="*", default=[], help="Explicit type definition files")
    parser.add_argument("-tf", "--type-folders", nargs="*", default=[], help="Folders containing type definitions")
    parser.add_argument("-o", "--output-dir", required=True, help="Output directory for the ROS package")
    args = parser.parse_args()

    type_paths = _collect_type_files(args.type_files, args.type_folders)
    if not type_paths:
        parser.error("No type files were provided")

    types = _load_types(type_paths)
    output_dir = pathlib.Path(args.output_dir)

    _create_project_structure(output_dir)
    _write_header(output_dir, types)
    _write_source(output_dir, types)
    _write_package_xml(output_dir)
    _write_cmake_lists(output_dir)
    return 0

if __name__ == "__main__":
    sys.exit(main())