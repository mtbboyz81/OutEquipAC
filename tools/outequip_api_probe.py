#!/usr/bin/env python3
import argparse
import asyncio
import datetime as dt
from typing import Any

import aioesphomeapi


HOST = "192.168.1.169"
PORT = 6053


def entity_kind(entity: Any) -> str:
    name = type(entity).__name__
    return (
        name.removeprefix("ListEntities")
        .removesuffix("Response")
        .removesuffix("Info")
        .lower()
    )


def entity_label(entity: Any) -> str:
    name = getattr(entity, "name", "") or getattr(entity, "object_id", "")
    return f"{entity_kind(entity)} key={getattr(entity, 'key', '?')} name={name!r}"


def state_value(state: Any) -> str:
    parts = []
    for attr in (
        "state",
        "missing_state",
        "current_temperature",
        "target_temperature",
        "mode",
        "fan_mode",
        "custom_fan_mode",
        "preset",
        "custom_preset",
        "action",
        "brightness",
        "color_mode",
    ):
        if hasattr(state, attr):
            parts.append(f"{attr}={getattr(state, attr)!r}")
    return ", ".join(parts) if parts else repr(state)


def find_entity(entities: list[Any], kind: str, name: str) -> Any:
    for entity in entities:
        if entity_kind(entity) == kind and getattr(entity, "name", "") == name:
            return entity
    raise SystemExit(f"Could not find {kind} entity named {name!r}")


def find_entity_by_name(entities: list[Any], name: str) -> Any:
    for entity in entities:
        if getattr(entity, "name", "") == name:
            return entity
    raise SystemExit(f"Could not find entity named {name!r}")


async def collect_states(client: aioesphomeapi.APIClient, seconds: float) -> dict[int, Any]:
    states: dict[int, Any] = {}

    def on_state(state: Any) -> None:
        states[getattr(state, "key")] = state

    client.subscribe_states(on_state)
    await asyncio.sleep(seconds)
    return states


def print_selected_states(label: str, entities: list[Any], states: dict[int, Any]) -> None:
    print(label)
    names = (
        "Thermostat",
        "Raw UART Power",
        "Raw UART Mode",
        "Raw UART Set Temperature",
        "Raw UART Fan Speed",
        "Raw UART Undervolt",
        "Raw UART Overvolt",
        "Raw UART Intake Temp",
        "Raw UART Outlet Temp",
        "Raw UART LCD",
        "Raw UART Swing",
        "Raw UART Voltage",
        "Raw UART Amperage",
        "Raw UART Light",
        "Raw UART Active",
        "UART Frames TX",
        "UART Frames RX",
        "UART Frames Failed",
    )
    for name in names:
        entity = next((ent for ent in entities if getattr(ent, "name", "") == name), None)
        if entity is None:
            continue
        state = states.get(entity.key)
        if state is None:
            print(f"  {name}: no state")
        else:
            print(f"  {name}: {state_value(state)}")


def should_watch(name: str) -> bool:
    return name in {
        "Thermostat",
        "Raw UART Power",
        "Raw UART Mode",
        "Raw UART Set Temperature",
        "Raw UART Fan Speed",
        "Raw UART LCD",
        "Raw UART Light",
        "Raw UART Active",
        "UART Frames Failed",
        "UART Spurious Bytes RX",
    }


async def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--seconds", type=float, default=5.0)
    parser.add_argument("--raw-key", type=int)
    parser.add_argument("--raw-value", type=int)
    parser.add_argument("--query", action="store_true")
    parser.add_argument("--send", action="store_true")
    parser.add_argument("--query-all", action="store_true")
    parser.add_argument("--fan-sweep", action="store_true")
    parser.add_argument("--mode-sweep", action="store_true")
    parser.add_argument("--eco-sequence", action="store_true")
    parser.add_argument("--set-temp-sweep", action="store_true")
    parser.add_argument("--watch", action="store_true")
    args = parser.parse_args()

    client = aioesphomeapi.APIClient(args.host, PORT, client_info="outequip-api-probe")
    await client.connect(login=True)
    try:
        info, entities, _ = await client.device_info_and_list_entities()
        by_key = {getattr(entity, "key"): entity for entity in entities}

        print(f"DEVICE {info.name} {info.mac_address} ESPHome {info.esphome_version}")
        print("ENTITIES")
        for entity in sorted(entities, key=lambda e: (entity_kind(e), getattr(e, "name", ""))):
            name = getattr(entity, "name", "")
            if name.startswith(("Raw UART", "UART Frames", "Thermostat", "Undervolt", "LCD", "Light")):
                print(f"  {entity_label(entity)}")

        if args.raw_key is not None:
            raw_key_number = find_entity(entities, "number", "Raw UART Key")
            client.number_command(raw_key_number.key, float(args.raw_key))
            await asyncio.sleep(0.5)

        if args.raw_value is not None:
            raw_value_number = find_entity(entities, "number", "Raw UART Value")
            client.number_command(raw_value_number.key, float(args.raw_value))
            await asyncio.sleep(0.5)

        if args.query:
            raw_query_button = find_entity(entities, "button", "Raw UART Query")
            client.button_command(raw_query_button.key)
            await asyncio.sleep(1.5)

        if args.send:
            raw_send_button = find_entity(entities, "button", "Raw UART Send")
            client.button_command(raw_send_button.key)
            await asyncio.sleep(1.5)

        states: dict[int, Any] = {}

        last_seen: dict[int, str] = {}

        def on_state(state: Any) -> None:
            states[getattr(state, "key")] = state
            if not args.watch:
                return
            entity = by_key.get(getattr(state, "key"))
            if entity is None:
                return
            name = getattr(entity, "name", "")
            if not should_watch(name):
                return
            value = state_value(state)
            if last_seen.get(entity.key) == value:
                return
            last_seen[entity.key] = value
            now = dt.datetime.now().strftime("%H:%M:%S")
            print(f"{now} {name}: {value}", flush=True)

        client.subscribe_states(on_state)
        await asyncio.sleep(args.seconds)

        async def raw_query(key: int, wait: float = 1.5) -> None:
            raw_key_number = find_entity(entities, "number", "Raw UART Key")
            raw_query_button = find_entity(entities, "button", "Raw UART Query")
            client.number_command(raw_key_number.key, float(key))
            await asyncio.sleep(0.3)
            client.button_command(raw_query_button.key)
            await asyncio.sleep(wait)

        async def raw_send(key: int, value: int, wait: float = 1.5) -> None:
            raw_key_number = find_entity(entities, "number", "Raw UART Key")
            raw_value_number = find_entity(entities, "number", "Raw UART Value")
            raw_send_button = find_entity(entities, "button", "Raw UART Send")
            client.number_command(raw_key_number.key, float(key))
            await asyncio.sleep(0.3)
            client.number_command(raw_value_number.key, float(value))
            await asyncio.sleep(0.3)
            client.button_command(raw_send_button.key)
            await asyncio.sleep(wait)

        if args.query_all:
            for key in (1, 2, 3, 4, 5, 6, 7, 8, 10, 16, 18, 19, 28, 66):
                await raw_query(key)
            print_selected_states("QUERY_ALL_SUMMARY", entities, states)

        if args.fan_sweep:
            fan_entity = find_entity_by_name(entities, "Raw UART Fan Speed")
            original_fan = states.get(fan_entity.key)
            original_fan_value = None
            if original_fan is not None and not getattr(original_fan, "missing_state", False):
                original_fan_value = int(getattr(original_fan, "state"))

            print_selected_states("FAN_SWEEP_BASELINE", entities, states)
            for value in (1, 2, 3, 4, 5):
                await raw_send(4, value)
                for key in (2, 3, 4):
                    await raw_query(key, wait=0.8)
                print_selected_states(f"FAN_SWEEP_VALUE_{value}", entities, states)

            if original_fan_value is not None:
                await raw_send(4, original_fan_value)
                for key in (2, 3, 4):
                    await raw_query(key, wait=0.8)
                print_selected_states(f"FAN_SWEEP_RESTORED_{original_fan_value}", entities, states)

        if args.mode_sweep:
            mode_entity = find_entity_by_name(entities, "Raw UART Mode")
            fan_entity = find_entity_by_name(entities, "Raw UART Fan Speed")
            original_mode = states.get(mode_entity.key)
            original_fan = states.get(fan_entity.key)
            original_mode_value = None
            original_fan_value = None
            if original_mode is not None and not getattr(original_mode, "missing_state", False):
                original_mode_value = int(getattr(original_mode, "state"))
            if original_fan is not None and not getattr(original_fan, "missing_state", False):
                original_fan_value = int(getattr(original_fan, "state"))

            print_selected_states("MODE_SWEEP_BASELINE", entities, states)
            for value, label in ((1, "COOL"), (4, "ECO"), (5, "SLEEP"), (6, "TURBO")):
                await raw_send(2, value)
                for key in (2, 3, 4):
                    await raw_query(key, wait=0.8)
                print_selected_states(f"MODE_SWEEP_{label}_{value}", entities, states)

            if original_mode_value is not None:
                await raw_send(2, original_mode_value)
                for key in (2, 3, 4):
                    await raw_query(key, wait=0.8)
                print_selected_states(f"MODE_SWEEP_RESTORED_MODE_{original_mode_value}", entities, states)

            if original_fan_value is not None:
                await raw_send(4, original_fan_value)
                for key in (2, 3, 4):
                    await raw_query(key, wait=0.8)
                print_selected_states(f"MODE_SWEEP_RESTORED_FAN_{original_fan_value}", entities, states)

        if args.eco_sequence:
            async def query_core(label: str) -> None:
                for key in (2, 3, 4):
                    await raw_query(key, wait=0.8)
                print_selected_states(label, entities, states)

            print_selected_states("ECO_SEQUENCE_START", entities, states)

            await raw_send(2, 1)
            await raw_send(4, 1)
            await query_core("ECO_BASELINE_COOL_FAN_1")

            await raw_send(2, 4)
            await query_core("ECO_MODE_4_ONLY")

            await raw_send(2, 1)
            await raw_send(4, 1)
            await query_core("ECO_RESET_COOL_FAN_1")

            await raw_send(3, 26)
            await raw_query(3, wait=1.2)
            await raw_send(2, 4)
            await query_core("ECO_SET_TEMP_26_THEN_MODE_4")

            await raw_send(2, 1)
            await raw_send(4, 1)
            await query_core("ECO_SEQUENCE_RESTORED_COOL_FAN_1")

        if args.set_temp_sweep:
            async def query_temp_context(label: str) -> None:
                for key in (2, 3, 4):
                    await raw_query(key, wait=0.8)
                print_selected_states(label, entities, states)

            print_selected_states("SET_TEMP_SWEEP_START", entities, states)
            for value in (24, 25, 26, 63, 72, 78, 79):
                await raw_send(3, value)
                await query_temp_context(f"SET_TEMP_WRITE_{value}")

        print("STATES")
        for key, state in sorted(states.items()):
            entity = by_key.get(key)
            if entity is None:
                continue
            name = getattr(entity, "name", "")
            if name.startswith(("Raw UART", "UART Frames", "Thermostat", "Undervolt", "LCD", "Light")):
                print(f"  {entity_label(entity)} -> {state_value(state)}")
    finally:
        await client.disconnect()


if __name__ == "__main__":
    asyncio.run(main())
