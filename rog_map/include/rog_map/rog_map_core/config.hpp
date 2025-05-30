/**
* This file is part of ROG-Map
*
* Copyright 2024 Yunfan REN, MaRS Lab, University of Hong Kong, <mars.hku.hk>
* Developed by Yunfan REN <renyf at connect dot hku dot hk>
* for more information see <https://github.com/hku-mars/ROG-Map>.
* If you use this code, please cite the respective publications as
* listed on the above website.
*
* ROG-Map is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ROG-Map is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with ROG-Map. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <rog_map/rog_map_core/common_lib.hpp>
#include <super_utils/yaml_loader.hpp>

#ifndef ORIGIN_AT_CORNER
#ifndef ORIGIN_AT_CENTER
#error "Please define either ORIGIN_AT_CORNER or ORIGIN_AT_CENTER, but not both."
#endif
#endif

#ifdef ORIGIN_AT_CORNER
#ifdef ORIGIN_AT_CENTER
#error "Cannot use both ORIGIN_AT_CENTER and ORIGIN_AT_CENTER at the same time. Please define only one."
#endif
#endif

namespace rog_map {
    using std::string;
    using std::vector;
    using color_text::RED;
    using color_text::BLUE;
    using color_text::RESET;
    typedef pcl::PointXYZI PclPoint;
    typedef pcl::PointCloud<PclPoint> PointCloud;

    class Config {
        static std::string replaceCmakeRootDir(const std::string &input) {
#define CMAKE_ROOT_DIR(name) (string(string(ROOT_DIR) + name))
            std::string replaced;
            std::regex cmakeRootDirRegex(R"(\$\{CMAKE_ROOT_DIR\}/)");
            if (std::regex_search(input, cmakeRootDirRegex)) {
                replaced = std::regex_replace(input, cmakeRootDirRegex, "");
                replaced = CMAKE_ROOT_DIR(replaced);
                printf("\033[0;32m The pcd_name is remapped to %s\033[0;0m \n", replaced.c_str());
            } else {
                replaced = input;
            }
            return replaced;
        }

    public:
        Config() {};

        Config(const string &cfg_path, const string &name_space = "rog_map") {
            yaml_loader::YamlLoader loader(cfg_path);
#ifdef ORIGIN_AT_CORNER
#ifdef ORIGIN_AT_CENTER
            throw std::runtime_error(" -- [SlidingMap]: ORIGIN_AT_CORNER and ORIGIN_AT_CENTER cannot be both true!");
#endif
#endif

#ifndef ORIGIN_AT_CORNER
#ifndef ORIGIN_AT_CENTER
            throw std::runtime_error(" -- [SlidingMap]: ORIGIN_AT_CORNER and ORIGIN_AT_CENTER cannot be both false!");
#endif
#endif

            loader.LoadParam(name_space + "/esdf/resolution", esdf_resolution, 0.2);
            loader.LoadParam(name_space + "/esdf/enable", esdf_en, false);
            vector<double> temp_esdf_update_box;
            loader.LoadParam(name_space + "/esdf/local_update_box", temp_esdf_update_box, temp_esdf_update_box);

            if (esdf_en) {
                if (temp_esdf_update_box.size() != 3) {
                    throw std::invalid_argument("Fix map origin size is not 3!");
                } else {
                    esdf_local_update_box = Vec3f(temp_esdf_update_box[0], temp_esdf_update_box[1],
                                                  temp_esdf_update_box[2]);
                }
            }


            loader.LoadParam(name_space + "/load_pcd_en", load_pcd_en, false);
            if (load_pcd_en) {
                loader.LoadParam(name_space + "/pcd_name", pcd_name, string("map.pcd"));
                pcd_name = replaceCmakeRootDir(pcd_name);
            }

            loader.LoadParam(name_space + "/map_sliding/enable", map_sliding_en, true);
            loader.LoadParam(name_space + "/map_sliding/threshold", map_sliding_thresh, -1.0);

            vector<double> temp_fix_origin;
            loader.LoadParam(name_space + "/fix_map_origin", temp_fix_origin, vector<double>{0, 0, 0});
            if (temp_fix_origin.size() != 3) {
                throw std::invalid_argument("Fix map origin size is not 3!");
            } else {
                fix_map_origin = Vec3f(temp_fix_origin[0], temp_fix_origin[1], temp_fix_origin[2]);
            }

            loader.LoadParam(name_space + "/frontier_extraction_en", frontier_extraction_en, false);

            loader.LoadParam(name_space + "/ros_callback/enable", ros_callback_en, false);
            loader.LoadParam(name_space + "/ros_callback/cloud_topic", cloud_topic, string("/cloud_registered"));
            loader.LoadParam(name_space + "/ros_callback/odom_topic", odom_topic, string("/lidar_slam/odom"));
            loader.LoadParam(name_space + "/ros_callback/odom_timeout", odom_timeout, 0.05);


            loader.LoadParam(name_space + "/visualization/enable", visualization_en, false);
            loader.LoadParam(name_space + "/visualization/use_dynamic_reconfigure", use_dynamic_reconfigure, false);
            loader.LoadParam(name_space + "/visualization/pub_unknown_map_en", pub_unknown_map_en, false);
            loader.LoadParam(name_space + "/visualization/frame_id", frame_id, string("world"));
            loader.LoadParam(name_space + "/visualization/time_rate", viz_time_rate, 0.0);
            loader.LoadParam(name_space + "/visualization/frame_rate", viz_frame_rate, 0);
            vector<double> temp_vis_range;
            loader.LoadParam(name_space + "/visualization/range", temp_vis_range, vector<double>{0, 0, 0});
            if (temp_vis_range.size() != 3) {
                throw std::invalid_argument("Visualization range size is not 3!");
            } else {
                visualization_range = Vec3f(temp_vis_range[0], temp_vis_range[1], temp_vis_range[2]);
                if (visualization_range.minCoeff() <= 0) {
                    std::cout << color_text::YELLOW <<
                              " -- [ROG] Visualization range is not set, visualization is disabled"
                              << color_text::RESET << std::endl;
                    visualization_en = false;
                }
            }


            loader.LoadParam(name_space + "/resolution", resolution, 0.1);
            loader.LoadParam(name_space + "/inflation_resolution", inflation_resolution, 0.1);
            /// Resize the map to ease indexing
            if (resolution > inflation_resolution) {
                throw std::invalid_argument("The inflation resolution should be equal or larger than the resolution!");
            }
            //    int scale = floor(inflation_resolution / resolution / 2);
            //    inflation_resolution = resolution * (scale * 2 + 1);
            //    ROS_ERROR("The inflation resolution is set to %f", inflation_resolution);


            /* For unk inflation */
            loader.LoadParam(name_space + "/unk_inflation_en", unk_inflation_en, false);
            loader.LoadParam(name_space + "/unk_inflation_step", unk_inflation_step, 1);

            loader.LoadParam(name_space + "/inflation_step", inflation_step, 1);
            loader.LoadParam(name_space + "/intensity_thresh", intensity_thresh, -1);

            vector<double> temp_map_size;
            loader.LoadParam(name_space + "/map_size", temp_map_size, vector<double>{10, 10, 0});
            if (temp_map_size.size() != 3) {
                throw std::invalid_argument("Map size dimension is not 3!");
            }
            map_size_d = Vec3f(temp_map_size[0], temp_map_size[1], temp_map_size[2]);


            loader.LoadParam(name_space + "/point_filt_num", point_filt_num, 2);
            if (point_filt_num <= 0) {
                std::cout <<  color_text::YELLOW << " -- [ROG] point_filt_num should be larger or equal than 1, it is set to 1 now."
                          << RESET << std::endl;
                point_filt_num = 1;
            }


            // raycasting
            loader.LoadParam(name_space + "/raycasting/enable", raycasting_en, true);
            loader.LoadParam(name_space + "/raycasting/batch_update_size", batch_update_size, 1);
            if (batch_update_size <= 0) {
                std::cout <<  color_text::YELLOW << " -- [ROG] batch_update_size should be larger or equal than 1, it is set to 1 now."
                          << RESET << std::endl;
                batch_update_size = 1;
            }
            loader.LoadParam(name_space + "/raycasting/unk_thresh", unk_thresh, 0.70);
            loader.LoadParam(name_space + "/raycasting/p_hit", p_hit, 0.70f);
            loader.LoadParam(name_space + "/raycasting/p_miss", p_miss, 0.70f);
            loader.LoadParam(name_space + "/raycasting/p_min", p_min, 0.12f);
            loader.LoadParam(name_space + "/raycasting/p_max", p_max, 0.97f);
            loader.LoadParam(name_space + "/raycasting/p_occ", p_occ, 0.80f);
            loader.LoadParam(name_space + "/raycasting/p_free", p_free, 0.30f);
            loader.LoadParam(name_space + "/raycasting/p_free", p_free, 0.30f);

            vector<double> temp_ray_range;
            loader.LoadParam(name_space + "/raycasting/ray_range", temp_ray_range, vector<double>{0.3, 10});
            if (temp_ray_range.size() != 2) {
                throw std::invalid_argument("Ray range size is not 2!");
            }
            raycast_range_min = temp_ray_range[0];
            raycast_range_max = temp_ray_range[1];
            sqr_raycast_range_max = raycast_range_max * raycast_range_max;
            sqr_raycast_range_min = raycast_range_min * raycast_range_min;
            vector<double> update_box;
            loader.LoadParam(name_space + "/raycasting/local_update_box", update_box, vector<double>{999, 999, 999});
            if (update_box.size() != 3) {
                throw std::invalid_argument("Update box size is not 3!");
            }
            local_update_box_d = Vec3f(update_box[0], update_box[1], update_box[2]);


            loader.LoadParam(name_space + "/virtual_ground_height", virtual_ground_height, -0.1);
            loader.LoadParam(name_space + "/virtual_ceil_height", virtual_ceil_height, -0.1);

            resetMapSize();

            /// Probabilistic Update
#define logit(x) (log((x) / (1 - (x))))
            l_hit = logit(p_hit);
            l_miss = logit(p_miss);
            l_min = logit(p_min);
            l_max = logit(p_max);
            l_occ = logit(p_occ);
            l_free = logit(p_free);

            int n_free = ceil(l_free / l_miss);
            int n_occ = ceil(l_occ / l_hit);

            std::cout << BLUE << "\t[ROG] n_free: " << n_free << RESET << std::endl;
            std::cout << BLUE << "\t[ROG] n_occ: " << n_occ << RESET << std::endl;

            std::cout << BLUE << "\t[ROG] l_hit: " << l_hit << RESET << std::endl;
            std::cout << BLUE << "\t[ROG] l_miss: " << l_miss << RESET << std::endl;
            std::cout << BLUE << "\t[ROG] l_min: " << l_min << RESET << std::endl;
            std::cout << BLUE << "\t[ROG] l_max: " << l_max << RESET << std::endl;
            std::cout << BLUE << "\t[ROG] l_occ: " << l_occ << RESET << std::endl;
            std::cout << BLUE << "\t[ROG] l_free: " << l_free << RESET << std::endl;

            // init spherical neighbor
            for (int dx = -inflation_step; dx <= inflation_step; dx++) {
                for (int dy = -inflation_step; dy <= inflation_step; dy++) {
                    for (int dz = -inflation_step; dz <= inflation_step; dz++) {
                        if (inflation_step == 1 ||
                            dx * dx + dy * dy + dz * dz <= inflation_step * inflation_step) {
                            inf_spherical_neighbor.emplace_back(dx, dy, dz);
                        }
                    }
                }
            }
            std::sort(inf_spherical_neighbor.begin(), inf_spherical_neighbor.end(), [](const Vec3i &a, const Vec3i &b) {
                return a.x() * a.x() + a.y() * a.y() + a.z() * a.z() < b.x() * b.x() + b.y() * b.y() + b.z() * b.z();
            });

            if (unk_inflation_en) {
                unk_inf_spherical_neighbor.clear();
                // init spherical neighbor
                for (int dx = -unk_inflation_step; dx <= unk_inflation_step; dx++) {
                    for (int dy = -unk_inflation_step; dy <= unk_inflation_step; dy++) {
                        for (int dz = -unk_inflation_step; dz <= unk_inflation_step; dz++) {
                            if (unk_inflation_step == 1 ||
                                dx * dx + dy * dy + dz * dz <= unk_inflation_step * unk_inflation_step) {
                                unk_inf_spherical_neighbor.emplace_back(dx, dy, dz);
                            }
                        }
                    }
                }
                std::sort(unk_inf_spherical_neighbor.begin(), unk_inf_spherical_neighbor.end(),
                          [](const Vec3i &a, const Vec3i &b) {
                              return a.x() * a.x() + a.y() * a.y() + a.z() * a.z() < b.x() * b.x() + b.y() * b.y() + b.
                                      z() * b.z();
                          });
            }

            /* init spherical neighbor for nearest neighbor search */
            constexpr double max_search_dis = 5.0;
            const int max_seach_step = ceil(max_search_dis / resolution);
            for (int dx = -max_seach_step; dx <= max_seach_step; dx++) {
                for (int dy = -max_seach_step; dy <= max_seach_step; dy++) {
                    for (int dz = -max_seach_step; dz <= max_seach_step; dz++) {
                        if (dx * dx + dy * dy + dz * dz <= max_seach_step * max_seach_step) {
                            spherical_neighbor.emplace_back(dx, dy, dz);
                        }
                    }
                }
            }
            std::sort(spherical_neighbor.begin(), spherical_neighbor.end(), [](const Vec3i &a, const Vec3i &b) {
                return a.x() * a.x() + a.y() * a.y() + a.z() * a.z() < b.x() * b.x() + b.y() * b.y() + b.z() * b.z();
            });
        }

        // add 24.07.18 add esdf
        bool esdf_en{false};
        Vec3f esdf_local_update_box{};
        double esdf_resolution{};

        bool load_pcd_en{false};
        bool use_dynamic_reconfigure{false};
        string pcd_name{"map.pcd"};

        double resolution{}, inflation_resolution{};
        int inflation_step{};
        Vec3f local_update_box_d, half_local_update_box_d{};
        Vec3i local_update_box_i, half_local_update_box_i{};
        Vec3f map_size_d, half_map_size_d{};
        Vec3i inf_half_map_size_i{}, half_map_size_i{}, fro_half_map_size_i{};

        /* The inf map virtual ceil should consider the inflation step
         *  the float type ceil and ground height is remap to the lower and upper bound of
         *  the inflation layer respectively
         */
        double virtual_ceil_height{}, virtual_ground_height{};
        int inf_virtual_ceil_height_id_g{}, inf_virtual_ground_height_id_g{};

        bool visualization_en{false}, frontier_extraction_en{false},
                raycasting_en{true}, ros_callback_en{false}, pub_unknown_map_en{false};

        /* Spherical neighbor for inflation*/
        std::vector<Vec3i> inf_spherical_neighbor{};
        std::vector<Vec3i> unk_inf_spherical_neighbor{};
        /* Spherical neighbor for nearest search within x m*/
        std::vector<Vec3i> spherical_neighbor{};

        /* intensity noise filter*/
        int intensity_thresh{};
        /* aster properties */
        string frame_id{};
        bool map_sliding_en{true};
        Vec3f fix_map_origin{};
        string odom_topic{}, cloud_topic{};
        /* probability update */
        double raycast_range_min{}, raycast_range_max{};
        double sqr_raycast_range_min{}, sqr_raycast_range_max{};
        int point_filt_num{}, batch_update_size{};
        float p_hit{}, p_miss{}, p_min{}, p_max{}, p_occ{}, p_free{};
        float l_hit{}, l_miss{}, l_min{}, l_max{}, l_occ{}, l_free{};

        /* for unknown inflation */
        bool unk_inflation_en{false};
        int unk_inflation_step{0};


        double odom_timeout{};
        Vec3f visualization_range{};
        double viz_time_rate{};
        int viz_frame_rate{};

        double unk_thresh{};
        double map_sliding_thresh{};

        void resetMapSize() {
            int inflation_ratio = ceil(inflation_resolution / resolution);

#ifdef ORIGIN_AT_CENTER
            // When discretized in such a way that the origin is at cell center
    // the inflation ratio should be an odd number
    if (inflation_ratio % 2 == 0) {
        inflation_ratio += 1;
    }
    std::cout << YELLOW << " -- [RM-Config] inflation_ratio: " << inflation_ratio << std::endl;
#endif

            inflation_resolution = resolution * inflation_ratio;
            std::cout << color_text::GREEN << " -- [RM-Config] inflation_resolution: " << inflation_resolution <<
                      color_text::RESET <<
                      std::endl;

            half_map_size_d = map_size_d / 2;

            // Size_d is only for calculate index number.
            // 1) we calculate the index number of the inf map
            int max_step = 0;
            if (!unk_inflation_en) {
                max_step = inflation_step;
            } else {
                max_step = std::max(inflation_step, unk_inflation_step);
            }
            inf_half_map_size_i = (half_map_size_d / inflation_resolution).cast<int>()
                                  + (max_step + 1) * Vec3i::Ones();

            // 2) we calculate the index number of the prob map, which should be smaller than infmap
            half_map_size_i = (inf_half_map_size_i - (max_step + 1) * Vec3i::Ones()) * inflation_ratio;

            // 3) compute the frontier counter map size
            if (frontier_extraction_en) {
                fro_half_map_size_i = half_map_size_i + Vec3i::Constant(1);
            }

            // 4) re-compute the map_size_d, the map_size_d is not that important.
            map_size_d = (half_map_size_i * 2 + Vec3i::Constant(1)).cast<double>() * resolution;
            half_map_size_d = map_size_d / 2;

            // 5) compute the index of raycasting update box
            half_local_update_box_d = local_update_box_d / 2;
            half_local_update_box_i = (half_local_update_box_d / resolution).cast<int>();
            local_update_box_i = half_local_update_box_i * 2 + Vec3i::Constant(1);
            local_update_box_d = local_update_box_i.cast<double>() * resolution;

            // 6) reset the virtual ground and ceil size
#ifdef ORIGIN_AT_CENTER
            inf_virtual_ceil_height_id_g = static_cast<int>(virtual_ceil_height / inflation_resolution + SIGN(
                virtual_ceil_height) * 0.5);
            inf_virtual_ground_height_id_g = static_cast<int>(virtual_ground_height / inflation_resolution + SIGN(
                virtual_ground_height) * 0.5);
            fmt::print(" -- [ROG-Map] Init, resetMapSize: ORIGIN_AT_CENTER.\n");
#endif

#ifdef ORIGIN_AT_CORNER
            inf_virtual_ceil_height_id_g = static_cast<int>(floor(virtual_ceil_height / inflation_resolution)) -
                                           inflation_step;
            inf_virtual_ground_height_id_g = static_cast<int>(floor(virtual_ground_height / inflation_resolution)) +
                                             inflation_step;

            fmt::print(" -- [ROG-Map] Init, resetMapSize: ORIGIN_AT_CORNER.\n");
#endif

            virtual_ceil_height = inf_virtual_ceil_height_id_g * inflation_resolution - 0.5 * inflation_resolution;
            virtual_ground_height = inf_virtual_ground_height_id_g * inflation_resolution + 0.5 * inflation_resolution;
            fmt::print(" -- [ROG-Map] Init, resetMapSize: virtual_ceil_height: {}, virtual_ground_height: {}.\n",
                       virtual_ceil_height, virtual_ground_height);
        }
    };
}
