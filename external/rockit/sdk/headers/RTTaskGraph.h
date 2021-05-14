/*
 * Copyright 2020 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *     author: <rimon.xu@rock-chips.com> and <martin.cheng@rock-chips.com>
 *       date: 2020-04-03
 *     module: RTTaskGraph
 */

#ifndef SRC_RT_TASK_APP_GRAPH_RTTASKGRAPH_H_
#define SRC_RT_TASK_APP_GRAPH_RTTASKGRAPH_H_

#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <stdarg.h>

#include "rt_header.h"
#include "rt_metadata.h"

typedef enum ExecutorMode {
    EXECUTOR_THREAD_POOL,
    EXECUTOR_THREAD_LOCAL,
} RTExecutorMode;

typedef enum GraphCmd {
    GRAPH_CMD_PREPARE = 100,
    GRAPH_CMD_START,
    GRAPH_CMD_STOP,
    GRAPH_CMD_RESUME,
    GRAPH_CMD_PAUSE,
    GRAPH_CMD_FLUSH,
    // for graph private command
    GRAPH_CMD_PRIVATE_CMD,
    // for task node command
    GRAPH_CMD_TASK_NODE_PRIVATE_CMD,
    GRAPH_CMD_MAX,
} RTGraphCmd;

class RTInputStreamManager;
class RTOutputStreamManager;
class RTTaskNode;
class RTGraphInputStream;
class RTGraphOutputStream;
class RTExecutor;
class RTScheduler;
class RTMediaBuffer;
class RTGraphParser;

class RTTaskGraph {
 public:
    explicit RTTaskGraph(const char* tagName);
    virtual ~RTTaskGraph();

 public:
    // external common api
    RT_RET      autoBuild(const char* configFile);
    RT_RET      prepare(RtMetaData *params = RT_NULL);
    RT_RET      start();
    RT_RET      stop(RT_BOOL block = RT_TRUE);
    RT_RET      resume();
    RT_RET      pause();
    RT_RET      flush();
    RT_RET      release();
    RT_RET      invoke(INT32 cmd, RtMetaData *params);
    RT_RET      observeOutputStream(const std::string& streamName,
                 INT32 streamId,
                 std::function<RT_RET(RTMediaBuffer *)> streamCallback);
    RT_RET      cancelObserveOutputStream(INT32 streamId);
    RT_RET      waitForObservedOutput();
    RT_RET      waitUntilDone();
    RT_RET      dump();

 public:
    // external link graph api
    RT_RET      addSubGraph(const char *graphConfigFile);
    RT_RET      removeSubGraph(const char *graphConfigFile);
    RT_RET      addDownGraph(RTTaskGraph *graph, std::string downStreamName, INT32 streamId);
    RT_RET      removeDownGraph(RTTaskGraph *graph, INT32 streamId);
    RT_RET      setupGraphInputStream(std::string streamName, INT32 downStreamId);

 public:
    // external link node api
    RTTaskNode* createNode(std::string nodeConfig, std::string streamConfig);
    RT_RET      linkNode(RTTaskNode *srcNode, RTTaskNode *dstNode);
    template <class T, class... Args>
    RT_RET      linkNode(T src, T dst, Args... rest) {
        return linkMultiNode(src, dst, rest...);
    }
    RT_RET      unlinkNode(RTTaskNode *srcNode, RTTaskNode *dstNode);

    RT_BOOL     hasLinkMode(std::string mode);
    RT_RET      selectLinkMode(std::string mode);
    template <class T, class... Args>
    RT_RET      selectLinkMode(T arg1, T arg2, Args... rest) {
        return selectMultiMode(arg1, arg2, rest...);
    }
    RT_RET      clearLinkShips();

 public:
    // internal api
    void        sendInterrupt(std::string reason);

 private:
    template <class T, class... Args>
    RT_RET      linkMultiNode(T src, T dst, Args... rest) {
        RT_RET ret = linkNode(src, dst);
        if (ret != RT_OK) {
            RT_LOGE("src node %d dst node %d link failed", src->getID(), dst->getID());
        } else {
            linkMultiNode(dst, rest...);
        }
        return ret;
    }

    template <class T>
    RT_RET      linkMultiNode(T end) {
        return RT_OK;
    }

    template <class T, class... Args>
    RT_RET      selectMultiMode(T arg1, T arg2, Args... rest) {
        RT_RET ret = selectLinkMode(arg1);
        if (ret != RT_OK) {
            RT_LOGE("select mode %s failed", arg1);
        } else {
            selectMultiMode(arg2, rest...);
        }
        return ret;
    }

    template <class T>
    RT_RET      selectMultiMode(T end) {
        RT_RET ret = selectLinkMode(end);
        if (ret != RT_OK) {
            RT_LOGE("select mode %s failed", end);
        }
        return ret;
    }

 private:
    RT_RET      autoLinkSource();
    RT_RET      autoUnlinkSource(INT32 nodeId);
    RT_RET      prepareForRun(RtMetaData *options);
    RT_RET      prepareNodeForRun(RTTaskNode *node, RtMetaData *options);
    RT_RET      cleanupAfterRun();
    void        updateThrottledNodes(RTInputStreamManager* stream, bool *streamWasFull);
    RT_RET      buildTaskNode(INT32 pipeId, INT32 nodeId, RTGraphParser* nodeParser);
    RT_RET      buildExecutors(RTGraphParser *parser);
    RT_RET      buildNodes(RTGraphParser *parser);
    RT_RET      buildLinkModes(RTGraphParser *parser);
    RT_RET      buildSubGraph(RTGraphParser *graphParser);
    RT_RET      addPacketToInputStream(std::string streamName, RTMediaBuffer *packet);
    RT_RET      notifyOutputStreams(RTGraphOutputStream *graphOutputStream);
    RTTaskNode *createNodeByText(const char *graphConfig);
    std::vector<std::vector<INT32>> parseLinkShip(std::string linkShip);
    RT_RET      linkNode(INT32 srcNodeId, INT32 dstNodeId);
    RT_RET      unlinkNode(INT32 srcNodeId, INT32 dstNodeId);

 protected:
    std::string     mTagName;
    RT_BOOL         mHasError;
    RtMutex         mFullInputStreamsMutex;
    RtMutex         mErrorMutex;
    RTScheduler    *mScheduler;

    std::map<INT32, RTExecutor *>                         mExecutors;
    std::map<INT32/* node id */, RTTaskNode *>            mNodes;
    std::map<INT32, RTInputStreamManager *>               mFullInputStreams;
    std::map<INT32/* node id */, RTInputStreamManager *>  mInputManagers;
    std::map<INT32/* node id */, RTOutputStreamManager *> mOutputManagers;
    // The graph output streams.
    std::map<INT32, RTGraphOutputStream *>                mGraphOutputStreams;
    std::map<std::string, RTGraphInputStream *>           mGraphInputStreams;
    std::map<std::string, std::string>                    mLinkShips;
    std::vector<std::string>                              mLinkModes;
};

#endif  // SRC_RT_TASK_APP_GRAPH_RTTASKGRAPH_H_
