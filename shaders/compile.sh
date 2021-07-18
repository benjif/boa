#!/bin/sh

glslc skybox/skybox.vert -o skybox/skybox_vert.spv
glslc skybox/skybox.frag -o skybox/skybox_frag.spv

glslc untextured/untextured.vert -o untextured/untextured_vert.spv
glslc untextured/untextured.frag -o untextured/untextured_frag.spv

glslc untextured_blinn_phong/untextured_blinn_phong.vert -o untextured_blinn_phong/untextured_blinn_phong_vert.spv
glslc untextured_blinn_phong/untextured_blinn_phong.frag -o untextured_blinn_phong/untextured_blinn_phong_frag.spv

glslc textured/textured.vert -o textured/textured_vert.spv
glslc textured/textured.frag -o textured/textured_frag.spv