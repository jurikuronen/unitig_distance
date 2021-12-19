#pragma once

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

#include "Queries.hpp"
#include "Timer.hpp"
#include "types.hpp"
#include "unitig_distance.hpp"

class OutlierTools {
public:
    OutlierTools(const Queries& queries,
                 const std::vector<real_t>& distance_vector,
                 const std::vector<int_t>& counts_vector, 
                 real_t max_distance,
                 bool output_one_based = false,
                 bool verbose = false)
    : m_queries(queries),
      m_distances(distance_vector),
      m_counts(counts_vector),
      m_max_distance(max_distance),
      m_output_one_based(output_one_based),
      m_verbose(verbose)
    { precalculate_largest_values(); }

    void determine_outliers(int_t ld_distance, int_t ld_distance_min, real_t ld_distance_score, int_t ld_distance_nth_score, int_t sgg_count_threshold) {
        // Linkage disequilibrium will be determined automatically if ld_distance >= 0.
        real_t a = (real_t) ld_distance < 0.0 ? ld_distance_min : ld_distance;
        real_t b = (real_t) ld_distance < 0.0 ? m_largest_distance : ld_distance;
        real_t required_score = ld_distance_score * m_largest_score;

        if (m_counts.size() == 0) sgg_count_threshold = 0;

        Timer t;

        int_t iter = 1;

        do {
            m_ld_distance = (a + b) / 2.0;
            real_t max_score = calculate_outlier_thresholds(ld_distance_nth_score, sgg_count_threshold);
            if (max_score < required_score) {
                b = m_ld_distance;
            } else {
                a = m_ld_distance;
            }
            std::cout << "OutlierTools: " << t.get_time_since_mark_and_set_mark() << ", Iteration " << iter++
                      << ", outlier threshold=" << m_outlier_threshold << ", extreme outlier threshold=" << m_extreme_outlier_threshold
                      << ", ld distance=" << (int_t) m_ld_distance << std::endl;
        } while (b - a > 50);

        collect_outliers(sgg_count_threshold);
    }

    void output_outliers(const std::string& outliers_filename, const std::string& outlier_stats_filename) {
        std::vector<int_t> indices;
        indices.reserve(m_outlier_indices.size() + m_extreme_outlier_indices.size());
        for (auto i : m_extreme_outlier_indices) indices.push_back(i);
        for (auto i : m_outlier_indices) indices.push_back(i);
        Queries q(m_queries, indices);
        q.output_distances(outliers_filename, q.get_distance_vector());

        std::ofstream ofs(outlier_stats_filename);
        ofs << m_ld_distance << ' ' << m_outlier_threshold << ' ' << m_extreme_outlier_threshold << '\n';
    }

    void print_details() const {
        std::cout << "OutlierTools: LD distance=" << (int_t) m_ld_distance << std::endl;
        std::cout << "OutlierTools: outlier threshold=" << m_outlier_threshold 
                  << " (" << m_outlier_indices.size() << " outliers)" << std::endl;
        std::cout << "OutlierTools: extreme outlier threshold=" << m_extreme_outlier_threshold
                  << " (" << m_extreme_outlier_indices.size() << " extreme outliers)" << std::endl;
    }

private:
    const Queries& m_queries;
    const std::vector<real_t>& m_distances;
    const std::vector<int_t>& m_counts;

    real_t m_max_distance;

    bool m_output_one_based;
    bool m_verbose;

    std::vector<int_t> m_outlier_indices;
    std::vector<int_t> m_extreme_outlier_indices;

    int_t m_largest_v = 0;
    real_t m_largest_distance = 0.0;
    real_t m_largest_score = 0.0;

    real_t m_ld_distance = -1.0;
    real_t m_outlier_threshold = -1.0;
    real_t m_extreme_outlier_threshold = -1.0;

    void precalculate_largest_values() {
        int_t largest_v = 0;
        real_t largest_distance = 0.0;
        real_t largest_score = 0.0;
        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            largest_v = std::max(largest_v, std::max(m_queries.v(i), m_queries.w(i)));
            largest_distance = std::max(largest_distance, unitig_distance::fixed_distance(m_distances[i], m_max_distance));
            largest_score = std::max(largest_score, m_queries.score(i));
        }
        m_largest_v = largest_v;
        m_largest_distance = largest_distance;
        m_largest_score = largest_score;
    }

    real_t calculate_outlier_thresholds(int_t ld_distance_nth_score, int_t sgg_count_threshold) {
        std::vector<real_t> v_scores(m_largest_v);

        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (sgg_count_threshold && m_counts[i] < sgg_count_threshold) continue;
            if (m_distances[i] <= m_ld_distance) continue;
            int_t v = m_queries.v(i);
            int_t w = m_queries.w(i);
            real_t score = m_queries.score(i);
            v_scores[v] = std::max(v_scores[v], score);
            v_scores[w] = std::max(v_scores[w], score);
        }

        std::vector<real_t> distribution;
        for (int_t i = 0; i < m_largest_v; ++i) if (v_scores[i] > 0.0) distribution.push_back(v_scores[i]);

        real_t q1 = get_q(distribution, 1);
        real_t q3 = get_q(distribution, 3);

        set_outlier_threshold(q1, q3);
        set_extreme_outlier_threshold(q1, q3);

        return max_score_from_end(distribution, ld_distance_nth_score);
    }

    real_t get_q(std::vector<real_t>& distribution, int_t q) const {
        int_t q_idx = q * distribution.size() / 4;
        std::nth_element(distribution.begin(), distribution.begin() + q_idx, distribution.end());
        return distribution[q_idx];
    }

    void set_outlier_threshold(real_t q1, real_t q3) { m_outlier_threshold = q3 + 1.5 * (q3 - q1); }
    void set_extreme_outlier_threshold(real_t q1, real_t q3) { m_extreme_outlier_threshold = q3 + 3.0 * (q3 - q1); }

    real_t max_score_from_end(std::vector<real_t>& distribution, int_t ld_distance_nth_score) {
        std::nth_element(distribution.begin(), distribution.begin() + ld_distance_nth_score, distribution.end(), std::greater<real_t>());
        return distribution[ld_distance_nth_score];
    }

    void collect_outliers(int_t sgg_count_threshold) {
        m_outlier_indices.clear();
        m_extreme_outlier_indices.clear();
        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (sgg_count_threshold && m_counts[i] < sgg_count_threshold) continue;
            real_t distance = m_distances[i];
            if (distance < m_ld_distance) continue;
            real_t score = m_queries.score(i);
            if (score < m_outlier_threshold) continue;
            if (score < m_extreme_outlier_threshold) m_outlier_indices.push_back(i);
            else m_extreme_outlier_indices.push_back(i);
        }
    }

};
