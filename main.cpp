
#define MCAP_IMPLEMENTATION
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "mcap/reader.hpp"
#include "DdsRecorderCommand/DdsRecorderCommandPubSubTypes.h"
#include "DdsRecorderStatus/DdsRecorderStatusPubSubTypes.h"


std::string input_file;

void help()
{
    std::cout << "Usage:  dds_replay "  << " -i input_file [-t tag_id]" << std::endl;
}


int parse_tag_list(std::vector<std::string>& tag_list)
{
    // parse the mcap file, get tag_list
    std::ifstream input(input_file, std::ios::binary);
    mcap::FileStreamReader dataSource{input};
    mcap::McapReader reader;
    auto status = reader.open(dataSource);
    if (!status.ok()) {
        printf("Failed to open file: %s\n", status.message.c_str());
        return -1;
    }
    auto onProblem = [](const mcap::Status& problem) {
        std::cerr << "! " << problem.message << "\n";
    };
    auto messages = reader.readMessages(onProblem);
    DdsRecorderCommandPubSubType cmdType;
    DdsRecorderStatusPubSubType statusType;
    int tag_index = 0;
    for (const auto& msgView : messages) {
        const mcap::Channel& channel = *msgView.channel;
        // std::cout << "[" << ToString(channel) << "] " << "\n";
        eprosima::fastrtps::rtps::SerializedPayload_t payload;
        payload.length = msgView.message.dataSize;
        payload.max_size = msgView.message.dataSize;
        payload.data = (unsigned char*)reinterpret_cast<const unsigned char*>(msgView.message.data);
        if (channel.topic == "/ddsrecorder/command") {
                DdsRecorderCommand cmd;
                cmdType.deserialize(&payload, &cmd);
                // printf("get cmd. log_time=%lu, cmd=%s,  args=%s\n", 
                //     msgView.message.logTime, cmd.command().c_str(), cmd.args().c_str());
                if (cmd.command() == "event") {
                    uint64_t log_time = msgView.message.logTime;
                    std::string event_args = cmd.args();
                    char time_str[64] = {0};
                    time_t timestamp = log_time / 1e9;
                    struct tm tm_time;
                    localtime_r(&timestamp, &tm_time);
                    strftime(time_str, sizeof(time_str), "%Y-%m-%d_%H-%M-%S", &tm_time);
                    printf("tag[%d] = %s, %s\n" , tag_index, time_str, event_args.c_str());
                    tag_list.push_back(time_str);
                    tag_index++;
                }
        }

        payload.data = nullptr;  // ps. to avoid memory delete twice.
        // std::cout << "[" << channel.topic << "] " << ToString(msgView.message) << "\n";
    }
    reader.close();

    return 0;
}


int dds_replay(int tag_id, const std::vector<std::string>& tag_list)
{
    // replay from tag_id
    std::string cmd = "ddsreplayer -i " + input_file;
    if (tag_id > 0) {
        if (tag_id >= tag_list.size()) {
            std::cout << "replay_id is out of range" << std::endl;
            return -1;
        }

        // 创建 replay yaml config 文件，写入 begin_time
        std::cout << "replay from tag" << tag_id << ": " << tag_list[tag_id] << std::endl;
        std::string replay_yaml;
        replay_yaml += "replayer:\n";
        replay_yaml += "    begin-time:\n";
        replay_yaml += "        datetime: " + tag_list[tag_id] + "\n";
        std::string replay_file = input_file.substr(0, input_file.find_last_of('.')) + ".replay.yaml";
        std::ofstream ofs(replay_file);
        ofs << replay_yaml;
        ofs.close();

        cmd += " -c " + replay_file;
    }
    std::cout << "run: " << cmd << std::endl;
    system(cmd.c_str());

    return 0;
}



int main(int argc, char* argv[]) {


    // -i 指定input mcap文件, 必须
    // -t id, 从指定的tag开始reolay
    if (argc < 3) {
        help();
        return -1;
    }

    // parse command line options
    int opt;
    int tag_id = 0;
    while ((opt = getopt(argc, argv, "i:t:"))!= -1) {
        switch (opt) {
            case 'i':
                std::cout << "input file: " << optarg << std::endl;
                input_file = optarg;
                break;
            case 't':
                tag_id = atoi(optarg);
                break;
            default:
                help();
                return -1;
        }
    }
    if (input_file.empty()) {
        std::cout << "input file is empty" << std::endl;
        help();
        return -1;
    }


    // parse mcap file, get tag_list
    std::vector<std::string> tag_list;
    if (parse_tag_list(tag_list) < 0) {
        return -1;
    }


    // replay from tag_id
    if (dds_replay(tag_id, tag_list) < 0 ) {
        return -1;
    }

    return 0;
}