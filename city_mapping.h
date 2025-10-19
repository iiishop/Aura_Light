#ifndef CITY_MAPPING_H
#define CITY_MAPPING_H

// City mapping structure
// Maps IP138 Chinese city names to standard English names
struct CityMapping
{
    const char *ip138Name; // Chinese name from IP138
    const char *cityName;  // Standard English name
};

// City mapping table
// IP138 returns location strings like:
// - "中国江苏省苏州市" → match "苏州" → return "Suzhou"
// - "英国英格兰伦敦" → match "伦敦" → return "London"
// - "中国内蒙古自治区呼和浩特市" → match "呼和浩特" → return "Hohhot"
//
// Matching principle: indexOf() - substring matching
// Works for 2-char, 3-char, or 4-char city names
const CityMapping CITY_MAP[] = {
    // ============================================
    // 中国城市 (Chinese Cities) - 39 cities
    // ============================================

    // 直辖市 (Municipalities)
    {"北京", "Beijing"},
    {"上海", "Shanghai"},
    {"天津", "Tianjin"},
    {"重庆", "Chongqing"},

    // 经济特区 (Special Economic Zones)
    {"深圳", "Shenzhen"},
    {"厦门", "Xiamen"},

    // 省会城市 (Provincial Capitals)
    {"广州", "Guangzhou"},      // 广东 Guangdong
    {"杭州", "Hangzhou"},       // 浙江 Zhejiang
    {"南京", "Nanjing"},        // 江苏 Jiangsu
    {"成都", "Chengdu"},        // 四川 Sichuan
    {"武汉", "Wuhan"},          // 湖北 Hubei
    {"西安", "Xi'an"},          // 陕西 Shaanxi
    {"福州", "Fuzhou"},         // 福建 Fujian
    {"济南", "Jinan"},          // 山东 Shandong
    {"郑州", "Zhengzhou"},      // 河南 Henan
    {"长沙", "Changsha"},       // 湖南 Hunan
    {"哈尔滨", "Harbin"},       // 黑龙江 Heilongjiang (3 chars)
    {"沈阳", "Shenyang"},       // 辽宁 Liaoning
    {"昆明", "Kunming"},        // 云南 Yunnan
    {"南昌", "Nanchang"},       // 江西 Jiangxi
    {"合肥", "Hefei"},          // 安徽 Anhui
    {"石家庄", "Shijiazhuang"}, // 河北 Hebei (3 chars)
    {"太原", "Taiyuan"},        // 山西 Shanxi
    {"长春", "Changchun"},      // 吉林 Jilin
    {"南宁", "Nanning"},        // 广西 Guangxi
    {"贵阳", "Guiyang"},        // 贵州 Guizhou
    {"兰州", "Lanzhou"},        // 甘肃 Gansu
    {"呼和浩特", "Hohhot"},     // 内蒙古 Inner Mongolia (4 chars!)
    {"乌鲁木齐", "Urumqi"},     // 新疆 Xinjiang (4 chars!)
    {"银川", "Yinchuan"},       // 宁夏 Ningxia
    {"西宁", "Xining"},         // 青海 Qinghai
    {"拉萨", "Lhasa"},          // 西藏 Tibet
    {"海口", "Haikou"},         // 海南 Hainan

    // 重要城市 (Major Cities)
    {"苏州", "Suzhou"},  // 江苏 (2 chars)
    {"青岛", "Qingdao"}, // 山东 (2 chars)
    {"大连", "Dalian"},  // 辽宁 (2 chars)

    // 特别行政区 (Special Administrative Regions)
    {"香港", "Hong Kong"},
    {"澳门", "Macau"},
    {"台北", "Taipei"},

    // ============================================
    // 国际城市 (International Cities) - 44 cities
    // ============================================

    // 英国 United Kingdom
    {"伦敦", "London"},

    // 美国 United States
    {"纽约", "New York"},
    {"洛杉矶", "Los Angeles"},
    {"旧金山", "San Francisco"},
    {"芝加哥", "Chicago"},
    {"西雅图", "Seattle"},
    {"波士顿", "Boston"},
    {"华盛顿", "Washington"},
    {"迈阿密", "Miami"},
    {"拉斯维加斯", "Las Vegas"},

    // 法国 France
    {"巴黎", "Paris"},

    // 德国 Germany
    {"柏林", "Berlin"},
    {"法兰克福", "Frankfurt"},
    {"慕尼黑", "Munich"},

    // 荷兰 Netherlands
    {"阿姆斯特丹", "Amsterdam"},

    // 比利时 Belgium
    {"布鲁塞尔", "Brussels"},

    // 瑞士 Switzerland
    {"苏黎世", "Zurich"},

    // 奥地利 Austria
    {"维也纳", "Vienna"},

    // 意大利 Italy
    {"罗马", "Rome"},
    {"米兰", "Milan"},

    // 西班牙 Spain
    {"马德里", "Madrid"},
    {"巴塞罗那", "Barcelona"},

    // 俄罗斯 Russia
    {"莫斯科", "Moscow"},
    {"圣彼得堡", "Saint Petersburg"},

    // 阿联酋 UAE
    {"迪拜", "Dubai"},

    // 日本 Japan
    {"东京", "Tokyo"},

    // 韩国 South Korea
    {"首尔", "Seoul"},

    // 新加坡 Singapore
    {"新加坡", "Singapore"},

    // 澳大利亚 Australia
    {"悉尼", "Sydney"},
    {"墨尔本", "Melbourne"},

    // 加拿大 Canada
    {"多伦多", "Toronto"},
    {"温哥华", "Vancouver"},

    // 泰国 Thailand
    {"曼谷", "Bangkok"},

    // 马来西亚 Malaysia
    {"吉隆坡", "Kuala Lumpur"},

    // 印度尼西亚 Indonesia
    {"雅加达", "Jakarta"},

    // 菲律宾 Philippines
    {"马尼拉", "Manila"},

    // 印度 India
    {"孟买", "Mumbai"},
    {"新德里", "New Delhi"},
    {"班加罗尔", "Bangalore"},

    // 埃及 Egypt
    {"开罗", "Cairo"},

    // 南非 South Africa
    {"约翰内斯堡", "Johannesburg"},

    // 巴西 Brazil
    {"圣保罗", "Sao Paulo"},
    {"里约热内卢", "Rio de Janeiro"},

    // 墨西哥 Mexico
    {"墨西哥城", "Mexico City"},

    // 阿根廷 Argentina
    {"布宜诺斯艾利斯", "Buenos Aires"},

    // 新西兰 New Zealand
    {"奥克兰", "Auckland"},
};

// Number of cities in the mapping table
const int CITY_MAP_SIZE = sizeof(CITY_MAP) / sizeof(CITY_MAP[0]);

#endif // CITY_MAPPING_H
