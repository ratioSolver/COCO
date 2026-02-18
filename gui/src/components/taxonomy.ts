import { h, VNode } from "snabbdom";
import { coco } from "../coco";
import * as echarts from 'echarts/core';
import { GraphChart } from "echarts/charts";

echarts.use([GraphChart]);

export function taxonomy(coco: coco.CoCo): VNode {
  const classes = Array.from(coco.get_classes().values()).map(cls => ({ name: cls.get_name() }));
  const links: { source: string, target: string }[] = [];
  for (const cls of coco.get_classes().values())
    for (const parent of cls.get_parents())
      links.push({ source: cls.get_name(), target: parent });

  const option: echarts.EChartsCoreOption = {
    series: [
      {
        type: 'graph',
        layout: 'force',
        data: classes,
        links,
        roam: true,
        label: {
          show: true,
          position: 'right'
        },
        force: {
          repulsion: 100,
          edgeLength: 50,
          gravity: 0.1
        }
      }
    ]
  };
  return h('div#taxonomy.flex-grow-1', {
    hook: {
      insert: (vnode) => {
        const chart = echarts.init(vnode.elm as HTMLDivElement);
        chart.setOption(option);
      }
    }
  });
}