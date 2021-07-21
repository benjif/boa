#!/bin/sh

glslc skybox/skybox.vert -o skybox/skybox_vert.spv
glslc skybox/skybox.frag -o skybox/skybox_frag.spv

glslc untextured/untextured.vert -o untextured/untextured_vert.spv
glslc untextured/untextured.frag -o untextured/untextured_frag.spv

glslc blinn_phong/blinn_phong.vert -o blinn_phong/blinn_phong_vert.spv
glslc blinn_phong/blinn_phong.frag -o blinn_phong/blinn_phong_frag.spv

glslc textured/textured.vert -o textured/textured_vert.spv
glslc textured/textured.frag -o textured/textured_frag.spv
