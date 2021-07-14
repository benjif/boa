#!/bin/sh

glslc skybox.vert -o skybox_vert.spv
glslc skybox.frag -o skybox_frag.spv

glslc untextured.vert -o untextured_vert.spv
glslc untextured.frag -o untextured_frag.spv

glslc textured.vert -o textured_vert.spv
glslc textured.frag -o textured_frag.spv