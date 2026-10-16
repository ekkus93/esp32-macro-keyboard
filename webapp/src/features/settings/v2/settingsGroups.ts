import type { IconName } from "../../../components/m3/Icon";
import type { SettingsGroup } from "../../../v2/routingV2";

/**
 * The Settings groups, in the order the list shows them (UXR-060). Each is
 * its own screen so a group's whole task fits a phone without a long page.
 * Diagnostics is listed beside them but is its own route, not a group.
 */
export interface SettingsGroupInfo {
  key: SettingsGroup;
  /** The app bar title and the list item's headline. */
  title: string;
  icon: IconName;
}

export const settingsGroupInfo: readonly SettingsGroupInfo[] = [
  { key: "device", title: "Device identity", icon: "keyboard" },
  { key: "network", title: "Network", icon: "wifi" },
  { key: "security", title: "Security", icon: "shield" },
  { key: "usb", title: "USB identity and confirmation", icon: "usb" },
  { key: "reset", title: "Restart and reset", icon: "restart" },
];

export function settingsGroupTitle(group: SettingsGroup): string {
  return settingsGroupInfo.find((info) => info.key === group)?.title ?? "";
}
