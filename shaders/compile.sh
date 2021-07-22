#!/bin/sh

glslc skybox/skybox.vert -o skybox/skybox_vert.spv
glslc skybox/skybox.frag -o skybox/skybox_frag.spv

glslc untextured/untextured.vert -o untextured/untextured_vert.spv
glslc untextured/untextured.frag -o untextured/untextured_frag.spv

glslc textured/textured.vert -o textured/textured_vert.spv
glslc textured/textured.frag -o textured/textured_frag.spv

glslc bounding_box/bounding_box.vert -o bounding_box/bounding_box_vert.spv
glslc bounding_box/bounding_box.frag -o bounding_box/bounding_box_frag.spv

glslc blinn_phong/untextured_blinn_phong.vert -o blinn_phong/untextured_blinn_phong_vert.spv
glslc blinn_phong/untextured_blinn_phong.frag -o blinn_phong/untextured_blinn_phong_frag.spv

glslc blinn_phong/textured_blinn_phong.vert -o blinn_phong/textured_blinn_phong_vert.spv
glslc blinn_phong/textured_blinn_phong.frag -o blinn_phong/textured_blinn_phong_frag.spv
