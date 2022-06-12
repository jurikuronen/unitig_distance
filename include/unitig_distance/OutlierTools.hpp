#pragma once

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

#include "DistanceVector.hpp"
#include "ProgramOptions.hpp"
#include "Queries.hpp"
#include "ResultsWriter.hpp"
#include "types.hpp"
#include "Utils.hpp"

class OutlierTools {
public:
    OutlierTools() = delete;
    OutlierTools(const Queries& queries, Timer& timer)
    : m_queries(queries),
      m_timer(timer),
      m_ld_distance(ProgramOptions::ld_distance),
      m_outlier_threshold(ProgramOptions::outlier_threshold),
      m_extreme_outlier_threshold(ProgramOptions::outlier_threshold)
    {
        if (ProgramOptions::has_operating_mode(OperatingMode::OUTLIER_TOOLS) && queries.extended_format()) calculate_query_values();
    }

    // Estimate outlier thresholds. Also estimate linkage disequilibrium distance if ld_distance < 0.
    void determine_and_output_outliers(const DistanceVector& dv, const std::string& outliers_filename, const std::string& outlier_stats_filename) {
        if (!m_queries.extended_format()) {
            std::cout << "    OutlierTools: No scores for unitig pairs available. Cannot determine outliers." << std::endl;
            return;
        }
        // Estimate ld distance if necessary.
        if (m_ld_distance < 0) {
            real_t largest_distance = calculate_largest_distance(dv);
            real_t min_distance = ProgramOptions::ld_distance_min;
            real_t required_score = ProgramOptions::ld_distance_score * m_largest_score;

            if (largest_distance < min_distance) {
                if (ProgramOptions::verbose) {
                    std::cout << "    OutlierTools: Distances in queries are smaller than the provided minimum ld distance (" << (int_t) largest_distance
                              << '<' << min_distance << "). Ignoring the given value." << std::endl;
                }
                min_distance = 0.0;
            }

            determine_ld_automatically(dv, min_distance, largest_distance, required_score);
        } else {
            // Use custom outlier threshold if required, otherwise calculate values with given ld distance.
            if (ProgramOptions::outlier_threshold >= 0.0) {
                m_outlier_threshold = m_extreme_outlier_threshold = ProgramOptions::outlier_threshold;
            } else {
                calculate_outlier_thresholds(dv);
            }
        }

        // Collect outliers.
        auto outlier_indices = collect_outliers(dv);

        // Output outliers.
        if (outlier_indices.size() > 0) {
            ResultsWriter::output_results(outliers_filename, m_queries, dv, outlier_indices);

            std::ofstream ofs(outlier_stats_filename);
            ofs << (int_t) m_ld_distance << ' ' << m_outlier_threshold << ' ' << m_extreme_outlier_threshold << ' ' << ProgramOptions::sgg_count_threshold << '\n';

            if (ProgramOptions::verbose) PrintUtils::print_tbss_tsmasm(m_timer, "Output outliers to files", outliers_filename, "and", outlier_stats_filename);
        } else if (ProgramOptions::verbose) {
            PrintUtils::print_tbss_tsmasm(m_timer, "No outliers could be collected with the current values.");
        }
    }

private:
    const Queries& m_queries;
    Timer& m_timer;

    int_t m_n_vs;
    int_t m_v_coverage;
    real_t m_ld_distance;
    real_t m_largest_score;
    real_t m_outlier_threshold;
    real_t m_extreme_outlier_threshold;

    void calculate_query_values() {
        std::unordered_set<int_t> vs;
        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            vs.insert(m_queries.v(i));
            vs.insert(m_queries.w(i));
            m_largest_score = std::max(m_largest_score, m_queries.score(i));
        }
        m_n_vs = vs.size();
    }

    real_t calculate_largest_distance(const DistanceVector& dv) const {
        real_t largest_distance = 0.0;
        for (auto d : dv) {
            if (d.count() < ProgramOptions::sgg_count_threshold) continue;
            largest_distance = std::max(largest_distance, Utils::fixed_distance(d, ProgramOptions::max_distance));
        }
        return largest_distance;
    }

    void determine_ld_automatically(const DistanceVector& dv, int_t a, int_t b, real_t required_score) {
        int_t iter = 0;
        while (b - a > 1) {
            m_ld_distance = (a + b) >> 1;
            real_t max_score = calculate_outlier_thresholds(dv);
            if (max_score < required_score) {
                b = m_ld_distance;
            } else {
                a = m_ld_distance;
            }
            if (ProgramOptions::verbose) {
                std::cout << "    OutlierTools: Iteration " << ++iter
                          << ", outlier threshold=" << m_outlier_threshold << ", extreme outlier threshold=" << m_extreme_outlier_threshold
                          << ", ld distance=" << (int_t) m_ld_distance
                          << ", coverage=" << m_v_coverage << " (" << Utils::neat_decimal_str(100 * m_v_coverage, m_n_vs) << "%)" << std::endl;
            }
        }
    }

    real_t calculate_outlier_thresholds(const DistanceVector& dv) {
        auto distribution = get_distribution(dv);
        if (distribution.size() == 0) return 0.0;

        real_t q1 = get_q(distribution, 1);
        real_t q3 = get_q(distribution, 3);

        set_outlier_threshold(q1, q3);
        set_extreme_outlier_threshold(q1, q3);

        m_v_coverage = distribution.size();

        return max_score_from_end(distribution);
    }

    std::vector<real_t> get_distribution(const DistanceVector& dv) const {
        std::vector<real_t> v_scores(m_queries.largest_v() + 1);

        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (dv[i].count() < ProgramOptions::sgg_count_threshold) continue;
            if (dv[i] < m_ld_distance) continue;
            int_t v = m_queries.v(i);
            int_t w = m_queries.w(i);
            real_t score = m_queries.score(i);
            v_scores[v] = std::max(v_scores[v], score);
            v_scores[w] = std::max(v_scores[w], score);
        }

        std::vector<real_t> distribution;
        for (auto score : v_scores) if (score > 0.0) distribution.push_back(score);
        return distribution;
    }

    real_t get_q(std::vector<real_t>& distribution, int_t q) {
        int_t q_idx = std::min(distribution.size() - 1, q * distribution.size() / 4);
        std::nth_element(distribution.begin(), distribution.begin() + q_idx, distribution.end());
        return distribution[q_idx];
    }

    real_t max_score_from_end(std::vector<real_t>& distribution) {
        int_t idx = std::min((int_t) distribution.size() - 1, ProgramOptions::ld_distance_nth_score);
        std::nth_element(distribution.begin(), distribution.begin() + idx, distribution.end(), std::greater<real_t>());
        return distribution[idx];
    }

    void set_outlier_threshold(real_t q1, real_t q3) { m_outlier_threshold = q3 + 1.5 * (q3 - q1); }
    void set_extreme_outlier_threshold(real_t q1, real_t q3) { m_extreme_outlier_threshold = q3 + 3.0 * (q3 - q1); }

    std::vector<int_t> collect_outliers(const DistanceVector& dv) const {
        std::vector<int_t> outlier_indices;
        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (dv[i].count() < ProgramOptions::sgg_count_threshold) continue;
            if (dv[i] < m_ld_distance) continue;
            if (m_queries.score(i) < m_outlier_threshold) continue;
            outlier_indices.push_back(i);
        }
        return outlier_indices;
    }

};

