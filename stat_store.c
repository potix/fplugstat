struct stat {
        time_t time;
        unsigned int humidity;
        unsigned int illuminance;
        double temperature;
        double rwatt;
};
typedef struct stat stat_t;

struct stat_store {
        stat_t *stats;
        unsigned long long  max_point;
        unsigned long long  current_point; /* 0 to max_point - 1 */
        int full; /* full flag */
};
