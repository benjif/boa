#!/bin/sh
glslc untextured.vert -o untextured_vert.spv
glslc untextured.frag -o untextured_frag.spv
glslc textured.vert -o textured_vert.spv
glslc textured.frag -o textured_frag.spv