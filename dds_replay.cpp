#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <json.hpp>



int main(int argc, char *argv[])
{

    int opt;

    // -i 指定input mcap文件, 必须
    // -r id, 从指定的tag开始reolay
    std::string input_file;
    int replay_id = 0;
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " -i input_file [-t id]" << std::endl;
        return -1;
    }
    while ((opt = getopt(argc, argv, "i:t:"))!= -1) {
        switch (opt) {
            case 'i':
                std::cout << "input file: " << optarg << std::endl;
                input_file = optarg;
                break;
            case 't':
                // std::cout << "replay from tag: " << optarg << std::endl;
                replay_id = atoi(optarg);
                break;
            default:
                std::cout << "Usage: " << argv[0] << " -i input_file [-t id]" << std::endl;
                return -1;
        }
    }

    // 判断 input_file 是否为空
    if (input_file.empty()) {
        std::cout << "can not find input file: " <<  input_file << std::endl;
        return -1;
    }


    // 读取与input_file同名， 后缀为tag的json文件
    std::string tag_file = input_file.substr(0, input_file.find_last_of('.')) + ".TAG";
    std::map<int, std::string> tag_map;
    if (tag_file.empty()) {
        std::cout << "no tag file: " <<  tag_file << std::endl;
        // return -1;
    } else {
         // 读取tag文件, 解析json数据
        std::ifstream tag_ifs(tag_file);
        if (!tag_ifs.is_open()) {
            std::cout << "cannot open tag file: " <<  tag_file << std::endl;
            return -1;
        }
        nlohmann::json tag_json;
        tag_ifs >> tag_json;
        tag_ifs.close();

        // 解析tag信息
        std::cout << "tag info: " << tag_file << std::endl;
        if (!tag_json["tags"].empty()) {
            for (auto& tag : tag_json["tags"]) {
                time_t timestamp = tag.value("timestamp_s", 0); 
                // convert timestamp to string, data format: yyyy-mm-dd hh:mm:ss.sss
                char time_str[64] = {0};
                struct tm tm_time;
                localtime_r(&timestamp, &tm_time);
                strftime(time_str, sizeof(time_str), "%Y-%m-%d_%H-%M-%S", &tm_time);

                printf("\t%d  %s  %s\n", (int)tag["id"], time_str, tag.value("comment", "").c_str());
                tag_map[(int)tag["id"]] = time_str;
            }
        }
    }


    std::string cmd = "ddsreplayer -i " + input_file;
    // check tag id 
    if (replay_id > 0) {
        if (tag_map.find(replay_id) == tag_map.end()) {
            std::cout << "no such tag id: " << replay_id << std::endl;
            return -1;
        }

        // 创建 replay yaml config 文件，写入 begin_time
        std::cout << "replay from tag: " << replay_id << " " << tag_map[replay_id] << std::endl;
        std::string replay_yaml;
        replay_yaml += "replayer:\n";
        replay_yaml += "    begin-time:\n";
        replay_yaml += "        datetime: " + tag_map[replay_id] + "\n";
        std::string replay_file = input_file.substr(0, input_file.find_last_of('.')) + ".replay.yaml";
        std::ofstream ofs(replay_file);
        ofs << replay_yaml;
        ofs.close();

        cmd += " -c " + replay_file;
    }


    // 启动dds replay 工具，指定yaml文件
    std::cout << "run: " << cmd << std::endl;
    system(cmd.c_str());

    return 0;
}





