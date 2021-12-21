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
      m_verbose(verbose),
      m_ok(true)
    { }

    void determine_outliers(int_t ld_distance, int_t ld_distance_min, real_t ld_distance_score, int_t ld_distance_nth_score, int_t sgg_count_threshold) {
        if (m_counts.size() == 0) sgg_count_threshold = 0;

        real_t largest_distance = get_largest_distance(sgg_count_threshold);
        if (largest_distance < ld_distance_min) {
            // Distances in queries not large enough.
            m_ok = false;
            std::cout << "OutlierTools: distances in queries not large enough (largest distance=" << largest_distance << "), maybe change parameters?" << std::endl;
            return;
        }

        // Linkage disequilibrium will be determined automatically if ld_distance >= 0.
        real_t a = (real_t) ld_distance < 0.0 ? ld_distance_min : ld_distance;
        real_t b = (real_t) ld_distance < 0.0 ? largest_distance : ld_distance;
        real_t required_score = ld_distance_score * m_queries.largest_score();;

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
                      << ", ld distance=" << (int_t) m_ld_distance
                      << ", coverage=" << m_v_coverage << " (" << unitig_distance::neat_decimal_str(m_v_coverage, m_queries.n_vs()) << ")" << std::endl;
        } while (b - a > 10);

        collect_outliers(sgg_count_threshold);
    }

    void output_outliers(const std::string& outliers_filename, const std::string& outlier_stats_filename) {
        if (!m_ok) return;
        std::vector<int_t> indices;
        indices.reserve(m_outlier_indices.size() + m_extreme_outlier_indices.size());
        for (auto i : m_extreme_outlier_indices) indices.push_back(i);
        for (auto i : m_outlier_indices) indices.push_back(i);
        Queries q(m_queries, indices);
        q.output_distances(outliers_filename, get_distances_from_indices(indices));

        std::ofstream ofs(outlier_stats_filename);
        ofs << (int_t) m_ld_distance << ' ' << m_outlier_threshold << ' ' << m_extreme_outlier_threshold << ' ' << m_v_coverage << '\n';
    }

    void print_details() const {
        if (!m_ok) {
            std::cout << "OutlierTools: unable to determine outliers." << std::endl; 
        } else {
            std::cout << "OutlierTools: LD distance=" << (int_t) m_ld_distance << std::endl;
            std::cout << "OutlierTools: outlier threshold=" << m_outlier_threshold 
                      << " (" << m_outlier_indices.size() << " outliers)" << std::endl;
            std::cout << "OutlierTools: extreme outlier threshold=" << m_extreme_outlier_threshold
                      << " (" << m_extreme_outlier_indices.size() << " extreme outliers)" << std::endl;
            std::cout << "OutlierTools: vertex coverage=" << m_v_coverage
                      << "(" << unitig_distance::neat_decimal_str(m_v_coverage, m_queries.n_vs()) << " queries covered)" << std::endl;
        }
    }

    bool ok() const { return m_ok; }

private:
    const Queries& m_queries;
    const std::vector<real_t>& m_distances;
    const std::vector<int_t>& m_counts;

    real_t m_max_distance;

    bool m_output_one_based;
    bool m_verbose;
    bool m_ok;

    std::vector<int_t> m_outlier_indices;
    std::vector<int_t> m_extreme_outlier_indices;

    real_t m_ld_distance = -1.0;
    real_t m_outlier_threshold = -1.0;
    real_t m_extreme_outlier_threshold = -1.0;
    int_t m_v_coverage = -1;

    real_t get_largest_distance(int_t sgg_count_threshold) const {
        real_t largest_distance = 0.0;
        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (sgg_count_threshold && m_counts[i] < sgg_count_threshold) continue;
            largest_distance = std::max(largest_distance, unitig_distance::fixed_distance(m_distances[i], m_max_distance));
        }
        return largest_distance;
    }

    real_t calculate_outlier_thresholds(int_t ld_distance_nth_score, int_t sgg_count_threshold) {
        int_t sz = m_queries.largest_v() + 1;
        std::vector<real_t> v_scores(sz);

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
        for (int_t i = 0; i < sz; ++i) if (v_scores[i] > 0.0) distribution.push_back(v_scores[i]);

        if (distribution.size() == 0) return 0.0;

        real_t q1 = get_q(distribution, 1);
        real_t q3 = get_q(distribution, 3);

        set_outlier_threshold(q1, q3);
        set_extreme_outlier_threshold(q1, q3);

        set_vertex_coverage(v_scores);

        return max_score_from_end(distribution, ld_distance_nth_score);
    }

    real_t get_q(std::vector<real_t>& distribution, int_t q) const {
        int_t q_idx = std::min(distribution.size() - 1, q * distribution.size() / 4);
        std::nth_element(distribution.begin(), distribution.begin() + q_idx, distribution.end());
        return distribution[q_idx];
    }

    void set_outlier_threshold(real_t q1, real_t q3) { m_outlier_threshold = q3 + 1.5 * (q3 - q1); }
    void set_extreme_outlier_threshold(real_t q1, real_t q3) { m_extreme_outlier_threshold = q3 + 3.0 * (q3 - q1); }

    void set_vertex_coverage(const std::vector<real_t>& v_scores) { m_v_coverage = 0; for (real_t score : v_scores) if (score > 0.0) ++m_v_coverage; }

    real_t max_score_from_end(std::vector<real_t>& distribution, int_t ld_distance_nth_score) const {
        ld_distance_nth_score = std::min((int_t) distribution.size() - 1, ld_distance_nth_score);
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

    std::vector<real_t> get_distances_from_indices(const std::vector<int_t>& indices) {
        std::vector<real_t> distances;
        distances.reserve(indices.size());
        for (auto i : indices) distances.push_back(m_distances[i]);
        return distances;
    }

};
