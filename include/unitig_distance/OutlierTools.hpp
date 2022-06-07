#pragma once

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

#include "DistanceVector.hpp"
#include "Queries.hpp"
#include "ResultsWriter.hpp"
#include "types.hpp"
#include "Utils.hpp"

class OutlierTools {
public:
    OutlierTools(const Queries& queries,
                 const DistanceVector& dv,
                 int_t sgg_count_threshold,
                 real_t max_distance = REAL_T_MAX,
                 bool verbose = false)
    : m_queries(queries),
      m_distances(dv),
      m_sgg_count_threshold(sgg_count_threshold),
      m_max_distance(max_distance),
      m_verbose(verbose),
      m_ok(true)
    { 
        std::unordered_set<int_t> vs;
        for (std::size_t i = 0; i < queries.size(); ++i) {
            vs.insert(queries.v(i));
            vs.insert(queries.w(i));
            m_largest_score = std::max(m_largest_score, queries.score(i));
        }
        m_n_vs = vs.size();

        m_largest_distance = 0.0;
        for (auto d : dv) {
            if (m_sgg_count_threshold && d.count() < m_sgg_count_threshold) continue;
            m_largest_distance = std::max(m_largest_distance, Utils::fixed_distance(d, m_max_distance));
        }
    }

    // Estimate outlier thresholds. Also estimate linkage disequilibrium distance if ld_distance < 0.
    void determine_outliers(int_t ld_distance,
                            int_t ld_distance_nth_score,
                            int_t ld_distance_min,
                            real_t ld_distance_score)
        {
        m_ld_distance = (real_t) ld_distance;

        if (m_ld_distance < 0.0) {
            if (m_largest_distance < ld_distance_min) {
                // Distances in queries not large enough.
                m_ok = false;
                m_reason = "distances in queries not large enough (largest distance=" + std::to_string((int_t) m_largest_distance)
                           + "<" + std::to_string(ld_distance_min) + "), maybe change parameters?";
                return;
            }

            determine_ld_automatically(ld_distance_min, m_largest_distance, ld_distance_score * m_largest_score, ld_distance_nth_score);
        }

        calculate_outlier_thresholds(ld_distance_nth_score);

        collect_outliers();
    }

    // Use custom values.
    void determine_outliers(int_t ld_distance, real_t outlier_threshold) {
        m_ld_distance = ld_distance;
        m_outlier_threshold = m_extreme_outlier_threshold = outlier_threshold;
        m_v_coverage = get_distribution().size();
        collect_outliers();
    }

    void output_outliers(const std::string& outliers_filename, const std::string& outlier_stats_filename) {
        if (!m_ok) return;

        std::vector<int_t> indices;
        indices.reserve(m_outlier_indices.size() + m_extreme_outlier_indices.size());
        for (auto i : m_extreme_outlier_indices) indices.push_back(i);
        for (auto i : m_outlier_indices) indices.push_back(i);

        Queries q(m_queries, indices);
        ResultsWriter::output_results(outliers_filename, Queries(m_queries, indices), DistanceVector(m_distances, indices), false, m_largest_distance);

        std::ofstream ofs(outlier_stats_filename);
        ofs << (int_t) m_ld_distance << ' ' << m_outlier_threshold << ' ' << m_extreme_outlier_threshold << ' ' << m_sgg_count_threshold << '\n';
    }

    void print_details() const {
        if (m_ok) { 
            std::cout << "OutlierTools: LD distance=" << (int_t) m_ld_distance << std::endl;
            std::cout << "OutlierTools: outlier threshold=" << m_outlier_threshold 
                      << " (" << m_outlier_indices.size() << " outliers)" << std::endl;
            std::cout << "OutlierTools: extreme outlier threshold=" << m_extreme_outlier_threshold
                      << " (" << m_extreme_outlier_indices.size() << " extreme outliers)" << std::endl;
            std::cout << "OutlierTools: vertex coverage=" << m_v_coverage
                      << " (" << Utils::neat_decimal_str(100 * m_v_coverage, m_n_vs) << "% queries covered)" << std::endl;
        }
    }

    bool ok() const { return m_ok; }
    const std::string& reason() const { return m_reason; }

private:
    const Queries& m_queries;
    const DistanceVector& m_distances;

    int_t m_sgg_count_threshold;

    real_t m_largest_distance;
    real_t m_max_distance;

    real_t m_largest_score;
    int_t m_n_vs;
    real_t m_max_score;

    bool m_verbose;
    bool m_ok;

    std::string m_reason;

    std::vector<int_t> m_outlier_indices;
    std::vector<int_t> m_extreme_outlier_indices;

    real_t m_ld_distance = -1.0;
    real_t m_outlier_threshold = -1.0;
    real_t m_extreme_outlier_threshold = -1.0;
    int_t m_v_coverage = -1;

    bool low_count(std::size_t idx) const { return m_sgg_count_threshold && m_distances[idx].count() < m_sgg_count_threshold; }

    void determine_ld_automatically(real_t a, real_t b, real_t required_score, int_t ld_distance_nth_score) {
        int_t iter = 1;
        for (m_ld_distance = (a + b) / 2.0; b - a > 2.0; m_ld_distance = (a + b) / 2.0) {
            real_t max_score = calculate_outlier_thresholds(ld_distance_nth_score);
            if (max_score < required_score) {
                b = m_ld_distance;
            } else {
                a = m_ld_distance;
            }
            if (m_verbose) {
                std::cout << "    OutlierTools: Iteration " << iter++
                          << ", outlier threshold=" << m_outlier_threshold << ", extreme outlier threshold=" << m_extreme_outlier_threshold
                          << ", ld distance=" << (int_t) m_ld_distance
                          << ", coverage=" << m_v_coverage << " (" << Utils::neat_decimal_str(100 * m_v_coverage, m_n_vs) << "%)" << std::endl;
            }
        }
    }

    real_t calculate_outlier_thresholds(int_t ld_distance_nth_score) {
        auto distribution = get_distribution();
        if (distribution.size() == 0) return 0.0;

        real_t q1 = get_q(distribution, 1);
        real_t q3 = get_q(distribution, 3);

        set_outlier_threshold(q1, q3);
        set_extreme_outlier_threshold(q1, q3);

        m_v_coverage = distribution.size();

        return max_score_from_end(distribution, ld_distance_nth_score);
    }

    std::vector<real_t> get_distribution() const {
        int_t sz = m_queries.largest_v() + 1;
        std::vector<real_t> v_scores(sz);

        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (low_count(i)) continue;
            if (m_distances[i] <= m_ld_distance) continue;
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

    real_t get_q(std::vector<real_t>& distribution, int_t q) const {
        int_t q_idx = std::min(distribution.size() - 1, q * distribution.size() / 4);
        std::nth_element(distribution.begin(), distribution.begin() + q_idx, distribution.end());
        return distribution[q_idx];
    }

    void set_outlier_threshold(real_t q1, real_t q3) { m_outlier_threshold = q3 + 1.5 * (q3 - q1); }
    void set_extreme_outlier_threshold(real_t q1, real_t q3) { m_extreme_outlier_threshold = q3 + 3.0 * (q3 - q1); }

    real_t max_score_from_end(std::vector<real_t>& distribution, int_t ld_distance_nth_score) const {
        ld_distance_nth_score = std::min((int_t) distribution.size() - 1, ld_distance_nth_score);
        std::nth_element(distribution.begin(), distribution.begin() + ld_distance_nth_score, distribution.end(), std::greater<real_t>());
        return distribution[ld_distance_nth_score];
    }

    void collect_outliers() {
        m_outlier_indices.clear();
        m_extreme_outlier_indices.clear();
        for (std::size_t i = 0; i < m_queries.size(); ++i) {
            if (low_count(i)) continue;
            real_t distance = m_distances[i];
            if (distance < m_ld_distance) continue;
            real_t score = m_queries.score(i);
            if (score < m_outlier_threshold) continue;
            if (score < m_extreme_outlier_threshold) m_outlier_indices.push_back(i);
            else m_extreme_outlier_indices.push_back(i);
        }
    }

};

