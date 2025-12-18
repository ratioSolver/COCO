#!/usr/bin/env python3
import argparse
import json
import sys
import pathlib
from typing import Dict, Any, List, Iterable

_TYPE_MAP = {
    "int": "int32",
    "float": "float32",
    "string": "string",
    "symbol": "string",
    "bool": "bool",
    "item": "string",
}

def _to_ros_identifier(symbol: str) -> str:
    result: List[str] = []
    cap_next = False
    for char in symbol:
        if char in {"_", " ", "-"}:
            cap_next = True
            continue
        if cap_next:
            result.append(char.upper())
            cap_next = False
        else:
            result.append(char)
    if not result or result[0].isdigit():
        result.insert(0, "T")
    else:
        result[0] = result[0].upper()
    return "".join(result)

def _prop_to_ros(name: str, definition: Dict[str, Any]) -> str:
    prop_type = definition.get("type")
    if prop_type not in _TYPE_MAP:
        raise ValueError(f"Unsupported property type '{prop_type}' for '{name}'")
    ros_type = _TYPE_MAP[prop_type]
    if definition.get("multiple"):
        ros_type += "[]"
    return f"{ros_type} {name}\n"

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

def _write_messages(output_dir: pathlib.Path, types: Dict[str, Any]) -> None:
    msg_dir = output_dir / "msg"
    msg_dir.mkdir(parents=True, exist_ok=True)
    for existing in msg_dir.glob("*.msg"):
        existing.unlink()
    for name in sorted(types):
        ros_name = _to_ros_identifier(name)
        msg_path = msg_dir / f"{ros_name}.msg"
        dynamic_props = types[name].get("dynamic_properties", {})
        with msg_path.open("w", encoding="utf-8") as handle:
            for prop_name in sorted(dynamic_props):
                handle.write(_prop_to_ros(prop_name, dynamic_props[prop_name]))

def _write_package_xml(output_dir: pathlib.Path) -> None:
    pkg_path = output_dir / "package.xml"
    with pkg_path.open("w", encoding="utf-8") as handle:
        handle.write("<?xml version=\"1.0\"?>\n")
        handle.write("<package format=\"3\">\n")
        handle.write("  <name>coco_ros_interfaces</name>\n")
        handle.write("  <version>0.1.0</version>\n")
        handle.write("  <description>COCO ROS Interfaces</description>\n")
        handle.write("  <maintainer email=\"riccardo.debenedictis@cnr.it\">Riccardo De Benedictis</maintainer>\n")
        handle.write("  <license>Apache-2.0</license>\n")
        handle.write("  <member_of_group>rosidl_interface_packages</member_of_group>\n")
        handle.write("  <buildtool_depend>ament_cmake</buildtool_depend>\n")
        handle.write("  <build_depend>rosidl_default_generators</build_depend>\n")
        handle.write("  <exec_depend>rosidl_default_runtime</exec_depend>\n")
        handle.write("</package>\n")

def _write_cmake_lists(output_dir: pathlib.Path, types: Dict[str, Any]) -> None:
    cmake_path = output_dir / "CMakeLists.txt"
    with cmake_path.open("w", encoding="utf-8") as handle:
        handle.write("cmake_minimum_required(VERSION 3.5)\n")
        handle.write("project(coco_ros_interfaces)\n\n")
        handle.write("find_package(ament_cmake REQUIRED)\n")
        handle.write("find_package(rosidl_default_generators REQUIRED)\n\n")
        handle.write("rosidl_generate_interfaces(${PROJECT_NAME}\n")
        for name in sorted(types):
            ros_name = _to_ros_identifier(name)
            handle.write(f"  \"msg/{ros_name}.msg\"\n")
        handle.write(")\n\n")
        handle.write("ament_export_dependencies(rosidl_default_runtime)\n\n")
        handle.write("ament_package()\n")

def main() -> int:
    parser = argparse.ArgumentParser(description="Generate ROS interface package from COCO types")
    parser.add_argument("-t", "--type-files", nargs="*", default=[], help="Explicit type definition files")
    parser.add_argument("-tf", "--type-folders", nargs="*", default=[], help="Folders containing type definitions")
    parser.add_argument("-o", "--output-dir", required=True, help="Output directory for the ROS package")
    args = parser.parse_args()

    type_paths = _collect_type_files(args.type_files, args.type_folders)
    if not type_paths:
        parser.error("No type files were provided")

    types = _load_types(type_paths)
    output_dir = pathlib.Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    _write_messages(output_dir, types)
    _write_package_xml(output_dir)
    _write_cmake_lists(output_dir, types)
    return 0

if __name__ == "__main__":
    sys.exit(main())